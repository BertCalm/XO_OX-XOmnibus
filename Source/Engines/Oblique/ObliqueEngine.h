#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
// ObliqueWavefolder — Sine wavefolder for harmonic grit.
// At low fold, gentle saturation. At high fold, metallic overtones.
// Run the Jewels' El-P production character: punchy but complex.
//==============================================================================
class ObliqueWavefolder
{
public:
    float process (float input, float foldAmount) noexcept
    {
        // foldAmount [0, 1] maps to [1, 6] fold iterations
        float gain = 1.0f + foldAmount * 5.0f;
        float x = input * gain;

        // Two-stage fold: sine fold + soft clip
        x = fastSin (x * 1.5707963f);  // pi/2 — maps [-1,1] to sine shape
        if (foldAmount > 0.3f)
        {
            // Second fold pass for extra harmonics at high settings
            float extra = (foldAmount - 0.3f) / 0.7f;
            x = fastSin (x * (1.5707963f + extra * 3.14159f));
        }
        return fastTanh (x);  // soft clip output
    }
};

//==============================================================================
// ObliqueBounce — Bouncing-ball rhythm generator.
//
// Models a ball dropping: each hit closer together and quieter.
// Creates the "ricocheting" percussive quality — accelerating clicks
// that decay like light bouncing between mirrors, losing energy
// at each reflection.
//
// Physics: interval[n] = initial_interval * ratio^n
//          velocity[n] = initial_velocity * damping^n
//==============================================================================
class ObliqueBounce
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        active = false;
        bounceIdx = 0;
        sampleCounter = 0;
        currentInterval = 0;
        clickLevel = 0.0f;
        clickPhase = 0.0f;
    }

    struct Params {
        float rate;       // Initial bounce interval (ms) [20-500]
        float gravity;    // Interval shrink ratio per bounce [0.3-0.95]
        float damping;    // Velocity decay per bounce [0.3-0.95]
        int   maxBounces; // Max bounce count [2-16]
        float swing;      // Swing amount [0-1] — offsets even bounces
        float clickTone;  // Click center frequency [200-8000 Hz]
    };

    void trigger (float velocity) noexcept
    {
        active = true;
        bounceIdx = 0;
        sampleCounter = 0;
        initialVelocity = velocity;
        fireClick (velocity);
    }

    // Returns click output sample (0-1)
    float process (const Params& p) noexcept
    {
        if (!active) return 0.0f;

        // Advance click decay
        clickLevel *= clickDecayRate;
        clickLevel = flushDenormal (clickLevel);
        clickPhase += clickPhaseInc;

        float out = clickLevel * fastSin (clickPhase);

        // Count samples to next bounce
        sampleCounter++;

        // Compute interval for current bounce
        float intervalMs = p.rate * std::pow (p.gravity, static_cast<float> (bounceIdx));

        // Apply swing to even-numbered bounces
        if (bounceIdx % 2 == 1)
            intervalMs *= (1.0f + p.swing * 0.4f);

        currentInterval = static_cast<int> (intervalMs * 0.001f * static_cast<float> (sr));
        if (currentInterval < 4) currentInterval = 4;

        if (sampleCounter >= currentInterval)
        {
            sampleCounter = 0;
            bounceIdx++;

            if (bounceIdx >= p.maxBounces || intervalMs < 5.0f)
            {
                active = false;
                return out;
            }

            // Fire next bounce click
            float vel = initialVelocity * std::pow (p.damping, static_cast<float> (bounceIdx));
            clickPhaseInc = p.clickTone * 6.28318530718f / static_cast<float> (sr);
            fireClick (vel);
        }

        return out;
    }

    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    bool active = false;
    int bounceIdx = 0;
    int sampleCounter = 0;
    int currentInterval = 0;
    float initialVelocity = 1.0f;

    // Click state
    float clickLevel = 0.0f;
    float clickPhase = 0.0f;
    float clickPhaseInc = 0.0f;
    float clickDecayRate = 0.99f;

    void fireClick (float velocity) noexcept
    {
        clickLevel = clamp (velocity, 0.0f, 1.0f);
        clickPhase = 0.0f;
        // Quick exponential decay: ~3ms at 44.1kHz
        float decaySamples = std::max (1.0f, static_cast<float> (sr) * 0.003f);
        clickDecayRate = std::pow (0.001f, 1.0f / decaySamples);
    }
};

//==============================================================================
// ObliquePrism — 6-tap spectral delay.
//
// The sonic prism: splits audio into 6 "facets," each a delay tap
// filtered at a different frequency (like a glass prism splitting
// white light into spectrum). Each facet has its own:
//   - Delay time (rhythmic spread)
//   - Bandpass filter center (spectral color)
//   - Pan position (stereo placement)
//   - Level (decreasing with distance from center)
//
// Feedback between facets creates kaleidoscopic multiplication —
// sound fragments bouncing between mirrors, each reflection
// picking up a new spectral color.
//==============================================================================
class ObliquePrism
{
public:
    static constexpr int kNumFacets = 6;
    static constexpr int kMaxDelaySamples = 96000; // ~2 sec at 48kHz

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePos = 0;

        // Default spectral colors — maps audio spectrum to light spectrum:
        //   Facet 0: Red    (100 Hz  — sub/bass warmth)
        //   Facet 1: Orange (300 Hz  — body/warmth)
        //   Facet 2: Yellow (800 Hz  — mid presence)
        //   Facet 3: Green  (2000 Hz — upper mid bite)
        //   Facet 4: Blue   (5000 Hz — presence/air)
        //   Facet 5: Violet (12000 Hz — sparkle/brilliance)
        static constexpr float defaultColors[kNumFacets] = {
            100.0f, 300.0f, 800.0f, 2000.0f, 5000.0f, 12000.0f
        };

        // Default pan positions: spread evenly L to R
        static constexpr float defaultPan[kNumFacets] = {
            0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f
        };

        float srf = static_cast<float> (sr);
        for (int i = 0; i < kNumFacets; ++i)
        {
            facetFilters[i].setMode (CytomicSVF::Mode::BandPass);
            facetFilters[i].setCoefficients (defaultColors[i], 0.4f, srf);
            facetFilters[i].reset();
            facetPan[i] = defaultPan[i];
        }

        feedbackFilter.setMode (CytomicSVF::Mode::LowPass);
        feedbackFilter.setCoefficients (8000.0f, 0.0f, srf);
        feedbackFilter.reset();
    }

    void reset() noexcept
    {
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePos = 0;
        for (int i = 0; i < kNumFacets; ++i)
            facetFilters[i].reset();
        feedbackFilter.reset();
        feedbackSample = 0.0f;
    }

    struct Params {
        float baseDelay;    // Base delay in ms [10-500]
        float spread;       // Time spread between facets [0-1]
        float colorSpread;  // How far apart the bandpass frequencies are [0-1]
        float stereoWidth;  // Pan spread [0-1]
        float feedback;     // Feedback amount [0-0.95]
        float mix;          // Dry/wet mix [0-1]
        float damping;      // Feedback HF damping [0-1] (darker with more bounces)
    };

    void process (float inputL, float inputR, float& outL, float& outR,
                  const Params& p) noexcept
    {
        float srf = static_cast<float> (sr);
        float mono = (inputL + inputR) * 0.5f;

        // Write input + feedback into delay buffer
        float fbSample = feedbackFilter.processSample (feedbackSample);
        fbSample = flushDenormal (fbSample);
        float writeVal = mono + fbSample * p.feedback;
        writeVal = fastTanh (writeVal);  // prevent feedback runaway

        delayBuffer[static_cast<size_t> (writePos)] = writeVal;

        // Update filter colors based on colorSpread
        // At spread=0, all filters converge to 1kHz
        // At spread=1, full spectrum spread
        static constexpr float centerFreqs[kNumFacets] = {
            100.0f, 300.0f, 800.0f, 2000.0f, 5000.0f, 12000.0f
        };

        for (int i = 0; i < kNumFacets; ++i)
        {
            float target = lerp (1000.0f, centerFreqs[i], p.colorSpread);
            facetFilters[i].setCoefficients (target, 0.35f + p.colorSpread * 0.3f, srf);
        }

        // Set feedback damping
        float dampFreq = lerp (16000.0f, 2000.0f, p.damping);
        feedbackFilter.setCoefficients (dampFreq, 0.0f, srf);

        // Read from each facet tap
        float wetL = 0.0f, wetR = 0.0f;
        float fbAccum = 0.0f;

        for (int i = 0; i < kNumFacets; ++i)
        {
            // Compute delay time for this facet
            // Facets spread in a fibonacci-like pattern for rhythmic interest
            static constexpr float facetRatios[kNumFacets] = {
                1.0f, 1.618f, 2.0f, 2.618f, 3.236f, 4.236f
            };
            float delayMs = p.baseDelay * lerp (1.0f, facetRatios[i], p.spread);
            int delaySamples = static_cast<int> (delayMs * 0.001f * srf);
            delaySamples = std::min (delaySamples, kMaxDelaySamples - 1);
            if (delaySamples < 1) delaySamples = 1;

            // Read from delay buffer
            int readPos = writePos - delaySamples;
            if (readPos < 0) readPos += kMaxDelaySamples;

            float tapSample = delayBuffer[static_cast<size_t> (readPos)];

            // Apply spectral color filter
            float colored = facetFilters[i].processSample (tapSample);

            // Level decreases for further facets (inverse distance)
            float facetLevel = 1.0f / (1.0f + static_cast<float> (i) * 0.3f);

            // Pan this facet
            float pan = lerp (0.5f, facetPan[i], p.stereoWidth);
            float panL = std::cos (pan * 1.5707963f);
            float panR = std::sin (pan * 1.5707963f);

            wetL += colored * facetLevel * panL;
            wetR += colored * facetLevel * panR;
            fbAccum += colored * facetLevel;
        }

        // Store feedback for next sample
        feedbackSample = flushDenormal (fbAccum / static_cast<float> (kNumFacets));

        // Advance write position
        writePos = (writePos + 1) % kMaxDelaySamples;

        // Mix dry/wet
        outL = inputL * (1.0f - p.mix) + wetL * p.mix;
        outR = inputR * (1.0f - p.mix) + wetR * p.mix;
    }

private:
    double sr = 44100.0;
    std::array<float, kMaxDelaySamples> delayBuffer {};
    int writePos = 0;

    CytomicSVF facetFilters[kNumFacets];
    float facetPan[kNumFacets] = {};

    CytomicSVF feedbackFilter;
    float feedbackSample = 0.0f;
};

//==============================================================================
// ObliquePhaser — 6-stage allpass phaser for Tame Impala psychedelic swirl.
// LFO-modulated allpass cascade creates sweeping notches.
// Applied post-prism for kaleidoscopic phase cancellation.
//==============================================================================
class ObliquePhaser
{
public:
    static constexpr int kNumStages = 6;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumStages; ++i)
        {
            stagesL[i].reset();
            stagesR[i].reset();
            stagesL[i].setMode (CytomicSVF::Mode::AllPass);
            stagesR[i].setMode (CytomicSVF::Mode::AllPass);
        }
        lfoPhase = 0.0;
    }

    struct Params {
        float rate;      // LFO rate [0.05-8 Hz]
        float depth;     // Sweep depth [0-1]
        float feedback;  // Feedback [0-0.95]
        float mix;       // Dry/wet [0-1]
    };

    void process (float& inOutL, float& inOutR, const Params& p) noexcept
    {
        float srf = static_cast<float> (sr);

        // LFO — sine with gentle triangle shaping for smoother sweep
        lfoPhase += static_cast<double> (p.rate) / sr;
        if (lfoPhase > 1.0) lfoPhase -= 1.0;
        float lfo = fastSin (static_cast<float> (lfoPhase * 6.28318530718));
        lfo = lfo * 0.5f + 0.5f;  // [0, 1]

        // Sweep range: 200 Hz - 4000 Hz modulated by depth
        float minFreq = 200.0f;
        float maxFreq = lerp (400.0f, 4000.0f, p.depth);
        float sweepFreq = lerp (minFreq, maxFreq, lfo);

        // Set allpass frequencies with slight spread between stages
        for (int i = 0; i < kNumStages; ++i)
        {
            float stageFreq = sweepFreq * (1.0f + static_cast<float> (i) * 0.15f);
            stageFreq = clamp (stageFreq, 20.0f, srf * 0.49f);
            stagesL[i].setCoefficients (stageFreq, 0.5f, srf);
            stagesR[i].setCoefficients (stageFreq, 0.5f, srf);
        }

        // Process through allpass cascade
        float wetL = inOutL + phaserFeedbackL * p.feedback;
        float wetR = inOutR + phaserFeedbackR * p.feedback;
        wetL = fastTanh (wetL);  // prevent feedback blowup
        wetR = fastTanh (wetR);

        for (int i = 0; i < kNumStages; ++i)
        {
            wetL = stagesL[i].processSample (wetL);
            wetR = stagesR[i].processSample (wetR);
        }

        phaserFeedbackL = flushDenormal (wetL);
        phaserFeedbackR = flushDenormal (wetR);

        // Mix
        inOutL = inOutL * (1.0f - p.mix) + wetL * p.mix;
        inOutR = inOutR * (1.0f - p.mix) + wetR * p.mix;
    }

private:
    double sr = 44100.0;
    double lfoPhase = 0.0;
    CytomicSVF stagesL[kNumStages];
    CytomicSVF stagesR[kNumStages];
    float phaserFeedbackL = 0.0f;
    float phaserFeedbackR = 0.0f;
};

//==============================================================================
// ObliqueVoice — Per-voice state.
//
// Each voice generates the "light beam" (sustained oscillator tone through
// wavefolder + filter) and fires a bouncing-ball percussion burst on note-on.
// The bounce clicks ride on top of the beam like percussive fragments
// ricocheting off mirrors.
//==============================================================================
struct ObliqueVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // Oscillators — dual osc for body + color
    PolyBLEP oscA;       // Primary body (Saw/Square/Pulse)
    PolyBLEP oscB;       // Secondary color (detuned for width)

    // Per-voice filter
    CytomicSVF filter;

    // Envelope (ADSR)
    float envLevel = 0.0f;
    float envStage = 0.0f;  // 0=attack, 1=decay, 2=sustain, 3=release
    bool releasing = false;

    // Voice stealing crossfade
    float fadeOutLevel = 0.0f;

    // Bounce (per-voice — each note gets its own ricochet)
    ObliqueBounce bounce;

    // Glide
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
};

//==============================================================================
// ObliqueEngine — Prismatic Bounce Engine ("XOblique")
//
// Run the Jewels × Funk/Disco × Tame Impala
//
// Prismatic light bouncing off mirrors — mirror ball, house of mirrors,
// kaleidoscope. Vibrant, ricocheting sound with hypnotic bounce.
// Bouncy percussive tops riding on top of light beams.
//
// Signal flow:
//   MIDI → Dual Oscillator → Wavefolder (grit) → Voice Filter → Envelope
//        + Bounce Engine (percussive ricochet clicks per note)
//        → Stereo Mix → Prism Delay (6-tap spectral split)
//        → Phaser (psychedelic swirl) → Output
//
// Accent: Prism Violet #BF40FF
// Param prefix: oblq_
//==============================================================================
class ObliqueEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //--------------------------------------------------------------------------
    // Lifecycle
    //--------------------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        prism.prepare (sr);
        phaser.prepare (sr);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& v : voices)
        {
            v.active = false;
            v.oscA.reset();
            v.oscB.reset();
            v.filter.reset();
            v.filter.setMode (CytomicSVF::Mode::LowPass);
            v.bounce.prepare (sr);
        }
    }

    void releaseResources() override
    {
        outputCacheL.clear();
        outputCacheR.clear();
    }

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.envLevel = 0.0f;
            v.fadeOutLevel = 0.0f;
            v.releasing = false;
            v.oscA.reset();
            v.oscB.reset();
            v.filter.reset();
            v.bounce.reset();
        }
        prism.reset();
        phaser.reset();
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //--------------------------------------------------------------------------
    // Audio rendering
    //--------------------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // ---- ParamSnapshot ----
        const int oscWaveIdx   = (pOscWave   != nullptr) ? static_cast<int> (pOscWave->load())   : 1;
        const float oscFold    = (pOscFold   != nullptr) ? pOscFold->load()   : 0.3f;
        const float oscDetune  = (pOscDetune != nullptr) ? pOscDetune->load() : 8.0f;
        const float percClick  = (pPercClick != nullptr) ? pPercClick->load() : 0.6f;
        const float percDecay  = (pPercDecay != nullptr) ? pPercDecay->load() : 0.3f;
        const float filterCut  = (pFilterCut != nullptr) ? pFilterCut->load() : 4000.0f;
        const float filterRes  = (pFilterRes != nullptr) ? pFilterRes->load() : 0.35f;
        const float attack     = (pAttack    != nullptr) ? pAttack->load()    : 0.005f;
        const float decay      = (pDecay     != nullptr) ? pDecay->load()     : 0.3f;
        const float sustain    = (pSustain   != nullptr) ? pSustain->load()   : 0.7f;
        const float release    = (pRelease   != nullptr) ? pRelease->load()   : 0.2f;
        const float bounceRate = (pBounceRate    != nullptr) ? pBounceRate->load()    : 80.0f;
        const float bounceGrav = (pBounceGravity != nullptr) ? pBounceGravity->load() : 0.7f;
        const float bounceDamp = (pBounceDamp    != nullptr) ? pBounceDamp->load()    : 0.75f;
        const int   bounceCnt  = (pBounceCnt     != nullptr) ? static_cast<int> (pBounceCnt->load()) : 6;
        const float bounceSwng = (pBounceSwing   != nullptr) ? pBounceSwing->load()   : 0.15f;
        const float clickTone  = (pClickTone     != nullptr) ? pClickTone->load()     : 3000.0f;
        const float prismDelay = (pPrismDelay    != nullptr) ? pPrismDelay->load()    : 80.0f;
        const float prismSprd  = (pPrismSpread   != nullptr) ? pPrismSpread->load()   : 0.6f;
        const float prismColor = (pPrismColor    != nullptr) ? pPrismColor->load()    : 0.7f;
        const float prismWidth = (pPrismWidth    != nullptr) ? pPrismWidth->load()    : 0.8f;
        const float prismFb    = (pPrismFeedback != nullptr) ? pPrismFeedback->load() : 0.4f;
        const float prismMix   = (pPrismMix      != nullptr) ? pPrismMix->load()      : 0.45f;
        const float prismDamp  = (pPrismDamp     != nullptr) ? pPrismDamp->load()     : 0.3f;
        const float phasRate   = (pPhaserRate     != nullptr) ? pPhaserRate->load()     : 0.5f;
        const float phasDepth  = (pPhaserDepth    != nullptr) ? pPhaserDepth->load()    : 0.6f;
        const float phasFb     = (pPhaserFeedback != nullptr) ? pPhaserFeedback->load() : 0.3f;
        const float phasMix    = (pPhaserMix      != nullptr) ? pPhaserMix->load()      : 0.4f;
        const float level      = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const float glide      = (pGlide != nullptr) ? pGlide->load() : 0.0f;

        // ---- Process MIDI ----
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        oscWaveIdx, oscDetune, glide, bounceRate, clickTone);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        // Consume coupling
        float pitchMod = externalPitchMod;
        float filterMod = externalFilterMod;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;

        // Hoist filter coefficient update
        float modCutoff = filterCut + filterMod * 4000.0f;
        modCutoff = clamp (modCutoff, 20.0f, 20000.0f);
        for (auto& v : voices)
        {
            if (!v.active) continue;
            v.filter.setCoefficients (modCutoff, filterRes, srf);
        }

        // ---- Render per-sample ----
        float peakEnv = 0.0f;

        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& v : voices)
            {
                if (!v.active) continue;

                // ---- Envelope ----
                float envTarget = 0.0f;
                if (!v.releasing)
                {
                    if (v.envStage < 1.0f)
                    {
                        // Attack
                        float attackRate = (attack > 0.001f) ? 1.0f / (attack * srf) : 1.0f;
                        v.envLevel += attackRate;
                        if (v.envLevel >= 1.0f)
                        {
                            v.envLevel = 1.0f;
                            v.envStage = 1.0f;
                        }
                    }
                    else if (v.envStage < 2.0f)
                    {
                        // Decay → sustain
                        float decayRate = (decay > 0.001f) ? 1.0f / (decay * srf) : 1.0f;
                        v.envLevel -= (v.envLevel - sustain) * decayRate;
                        v.envLevel = flushDenormal (v.envLevel);
                        if (v.envLevel <= sustain + 0.001f)
                        {
                            v.envLevel = sustain;
                            v.envStage = 2.0f;
                        }
                    }
                    // Sustain: hold at sustain level
                }
                else
                {
                    // Release
                    float relRate = (release > 0.001f) ? 1.0f / (release * srf) : 1.0f;
                    v.envLevel -= v.envLevel * relRate;
                    v.envLevel = flushDenormal (v.envLevel);
                    if (v.envLevel < 0.001f)
                    {
                        v.envLevel = 0.0f;
                        v.active = false;
                        continue;
                    }
                }

                // Voice stealing crossfade
                float stealFade = 1.0f;
                if (v.fadeOutLevel > 0.0f)
                {
                    v.fadeOutLevel -= 1.0f / (0.005f * srf);
                    if (v.fadeOutLevel <= 0.0f) v.fadeOutLevel = 0.0f;
                    stealFade = 1.0f - v.fadeOutLevel;
                }

                // ---- Glide ----
                if (glide > 0.001f)
                {
                    float glideRate = 1.0f / (glide * srf * 0.5f);
                    float diff = v.targetFreq - v.currentFreq;
                    v.currentFreq += diff * glideRate;
                }
                else
                {
                    v.currentFreq = v.targetFreq;
                }
                float freq = v.currentFreq * std::pow (2.0f, pitchMod);

                // ---- Dual Oscillator ----
                v.oscA.setFrequency (freq, srf);
                v.oscB.setFrequency (freq * std::pow (2.0f, oscDetune / 1200.0f), srf);

                float oscOut = v.oscA.processSample() * 0.7f
                             + v.oscB.processSample() * 0.3f;

                // ---- Wavefolder (RTJ grit) ----
                oscOut = wavefolder.process (oscOut, oscFold);

                // ---- Voice filter ----
                oscOut = v.filter.processSample (oscOut);

                // ---- Apply envelope ----
                float voiced = oscOut * v.envLevel * v.velocity * stealFade;

                // ---- Bounce clicks (percussive tops) ----
                ObliqueBounce::Params bp;
                bp.rate = bounceRate;
                bp.gravity = bounceGrav;
                bp.damping = bounceDamp;
                bp.maxBounces = bounceCnt;
                bp.swing = bounceSwng;
                bp.clickTone = clickTone;

                float bounceOut = v.bounce.process (bp) * percClick * v.velocity;

                // ---- Mix voice: body (center-ish) + bounce (wider) ----
                float bodyL = voiced * 0.7f;
                float bodyR = voiced * 0.7f;
                // Bounce clicks get slight per-voice pan scatter
                float bPan = 0.3f + (static_cast<float> (v.noteNumber % 7) / 7.0f) * 0.4f;
                float bounceL = bounceOut * std::cos (bPan * 1.5707963f);
                float bounceR = bounceOut * std::sin (bPan * 1.5707963f);

                mixL += bodyL + bounceL;
                mixR += bodyR + bounceR;

                if (v.envLevel > peakEnv) peakEnv = v.envLevel;
            }

            // ---- Prism Delay (kaleidoscopic multiplication) ----
            ObliquePrism::Params pp;
            pp.baseDelay = prismDelay;
            pp.spread = prismSprd;
            pp.colorSpread = prismColor;
            pp.stereoWidth = prismWidth;
            pp.feedback = prismFb;
            pp.mix = prismMix;
            pp.damping = prismDamp;

            float prismL = 0.0f, prismR = 0.0f;
            prism.process (mixL, mixR, prismL, prismR, pp);

            // ---- Phaser (Tame Impala swirl) ----
            ObliquePhaser::Params php;
            php.rate = phasRate;
            php.depth = phasDepth;
            php.feedback = phasFb;
            php.mix = phasMix;
            phaser.process (prismL, prismR, php);

            // ---- Output ----
            float outL = prismL * level;
            float outR = prismR * level;

            auto si = static_cast<size_t> (s);
            if (si < outputCacheL.size())
            {
                outputCacheL[si] = outL;
                outputCacheR[si] = outR;
            }

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, s, outL);
                buffer.addSample (1, s, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, s, (outL + outR) * 0.5f);
            }
        }

        envelopeOutput = peakEnv;
    }

    //--------------------------------------------------------------------------
    // Coupling
    //--------------------------------------------------------------------------

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
                externalFilterMod += amount;
                break;
            case CouplingType::AmpToPitch:
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 0.5f;
                break;
            case CouplingType::EnvToDecay:
                // Modulate prism feedback from external envelope
                externalFilterMod += amount * 0.3f;
                break;
            case CouplingType::RhythmToBlend:
                // Rhythm coupling modulates prism spread
                externalFilterMod += amount * 0.2f;
                break;
            default:
                break;
        }
    }

    //--------------------------------------------------------------------------
    // Parameters — prefix: oblq_
    //--------------------------------------------------------------------------

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
        // --- Oscillator ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "oblq_oscWave", 1 }, "Oblique Osc Wave",
            juce::StringArray { "Sine", "Saw", "Square", "Pulse", "Triangle" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_oscFold", 1 }, "Oblique Wavefold",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_oscDetune", 1 }, "Oblique Detune",
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 8.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_level", 1 }, "Oblique Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_glide", 1 }, "Oblique Glide",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Bounce (percussive ricochet) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_percClick", 1 }, "Oblique Click Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_percDecay", 1 }, "Oblique Click Decay",
            juce::NormalisableRange<float> (0.001f, 0.05f, 0.001f), 0.003f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_bounceRate", 1 }, "Oblique Bounce Rate",
            juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.4f), 80.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_bounceGravity", 1 }, "Oblique Bounce Gravity",
            juce::NormalisableRange<float> (0.3f, 0.95f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_bounceDamp", 1 }, "Oblique Bounce Damping",
            juce::NormalisableRange<float> (0.3f, 0.95f, 0.01f), 0.75f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_bounceCnt", 1 }, "Oblique Bounce Count",
            juce::NormalisableRange<float> (2.0f, 16.0f, 1.0f), 6.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_bounceSwing", 1 }, "Oblique Bounce Swing",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_clickTone", 1 }, "Oblique Click Tone",
            juce::NormalisableRange<float> (200.0f, 8000.0f, 1.0f, 0.3f), 3000.0f));

        // --- Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_filterCut", 1 }, "Oblique Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 4000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_filterRes", 1 }, "Oblique Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));

        // --- Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_attack", 1 }, "Oblique Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.3f), 0.005f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_decay", 1 }, "Oblique Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_sustain", 1 }, "Oblique Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_release", 1 }, "Oblique Release",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.2f));

        // --- Prism Delay ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismDelay", 1 }, "Oblique Prism Delay",
            juce::NormalisableRange<float> (10.0f, 500.0f, 0.1f, 0.35f), 80.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismSpread", 1 }, "Oblique Prism Spread",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismColor", 1 }, "Oblique Prism Color",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismWidth", 1 }, "Oblique Prism Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismFeedback", 1 }, "Oblique Prism Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismMix", 1 }, "Oblique Prism Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.45f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_prismDamp", 1 }, "Oblique Prism Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // --- Phaser ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_phaserRate", 1 }, "Oblique Phaser Rate",
            juce::NormalisableRange<float> (0.05f, 8.0f, 0.01f, 0.35f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_phaserDepth", 1 }, "Oblique Phaser Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_phaserFeedback", 1 }, "Oblique Phaser Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_phaserMix", 1 }, "Oblique Phaser Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscWave        = apvts.getRawParameterValue ("oblq_oscWave");
        pOscFold        = apvts.getRawParameterValue ("oblq_oscFold");
        pOscDetune      = apvts.getRawParameterValue ("oblq_oscDetune");
        pLevel          = apvts.getRawParameterValue ("oblq_level");
        pGlide          = apvts.getRawParameterValue ("oblq_glide");
        pPercClick      = apvts.getRawParameterValue ("oblq_percClick");
        pPercDecay      = apvts.getRawParameterValue ("oblq_percDecay");
        pBounceRate     = apvts.getRawParameterValue ("oblq_bounceRate");
        pBounceGravity  = apvts.getRawParameterValue ("oblq_bounceGravity");
        pBounceDamp     = apvts.getRawParameterValue ("oblq_bounceDamp");
        pBounceCnt      = apvts.getRawParameterValue ("oblq_bounceCnt");
        pBounceSwing    = apvts.getRawParameterValue ("oblq_bounceSwing");
        pClickTone      = apvts.getRawParameterValue ("oblq_clickTone");
        pFilterCut      = apvts.getRawParameterValue ("oblq_filterCut");
        pFilterRes      = apvts.getRawParameterValue ("oblq_filterRes");
        pAttack         = apvts.getRawParameterValue ("oblq_attack");
        pDecay          = apvts.getRawParameterValue ("oblq_decay");
        pSustain        = apvts.getRawParameterValue ("oblq_sustain");
        pRelease        = apvts.getRawParameterValue ("oblq_release");
        pPrismDelay     = apvts.getRawParameterValue ("oblq_prismDelay");
        pPrismSpread    = apvts.getRawParameterValue ("oblq_prismSpread");
        pPrismColor     = apvts.getRawParameterValue ("oblq_prismColor");
        pPrismWidth     = apvts.getRawParameterValue ("oblq_prismWidth");
        pPrismFeedback  = apvts.getRawParameterValue ("oblq_prismFeedback");
        pPrismMix       = apvts.getRawParameterValue ("oblq_prismMix");
        pPrismDamp      = apvts.getRawParameterValue ("oblq_prismDamp");
        pPhaserRate     = apvts.getRawParameterValue ("oblq_phaserRate");
        pPhaserDepth    = apvts.getRawParameterValue ("oblq_phaserDepth");
        pPhaserFeedback = apvts.getRawParameterValue ("oblq_phaserFeedback");
        pPhaserMix      = apvts.getRawParameterValue ("oblq_phaserMix");
    }

    //--------------------------------------------------------------------------
    // Identity
    //--------------------------------------------------------------------------

    juce::String getEngineId() const override { return "Oblique"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFBF40FF); }
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        return count;
    }

private:
    double sr = 44100.0;
    float srf = 44100.0f;

    // Voice management
    std::array<ObliqueVoice, kMaxVoices> voices {};

    // Shared DSP
    ObliqueWavefolder wavefolder;
    ObliquePrism prism;
    ObliquePhaser phaser;

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    //--------------------------------------------------------------------------
    // MIDI handlers
    //--------------------------------------------------------------------------

    void noteOn (int noteNumber, float velocity, int oscWaveIdx, float detuneCents,
                 float glide, float bounceRate, float clickTone) noexcept
    {
        // Find free voice or steal oldest
        int voiceIdx = -1;
        uint64_t oldestTime = UINT64_MAX;
        int oldestIdx = 0;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[static_cast<size_t> (i)].active)
            {
                voiceIdx = i;
                break;
            }
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldestIdx = i;
            }
        }

        if (voiceIdx < 0)
        {
            voiceIdx = oldestIdx;
            voices[static_cast<size_t> (voiceIdx)].fadeOutLevel = 1.0f;
        }

        auto& v = voices[static_cast<size_t> (voiceIdx)];
        v.active = true;
        v.noteNumber = noteNumber;
        v.velocity = velocity;
        v.startTime = ++voiceCounter;
        v.releasing = false;
        v.envStage = 0.0f;
        v.envLevel = 0.0f;

        // Set oscillator waveform
        PolyBLEP::Waveform wf;
        switch (oscWaveIdx)
        {
            case 0:  wf = PolyBLEP::Waveform::Sine;     break;
            case 1:  wf = PolyBLEP::Waveform::Saw;      break;
            case 2:  wf = PolyBLEP::Waveform::Square;   break;
            case 3:  wf = PolyBLEP::Waveform::Pulse;    break;
            case 4:  wf = PolyBLEP::Waveform::Triangle; break;
            default: wf = PolyBLEP::Waveform::Saw;      break;
        }
        v.oscA.setWaveform (wf);
        v.oscB.setWaveform (wf);

        // Frequency
        float freq = midiToFreq (noteNumber);
        if (glide > 0.001f && v.currentFreq > 10.0f)
            v.targetFreq = freq;  // glide from current position
        else
        {
            v.currentFreq = freq;
            v.targetFreq = freq;
        }

        // Trigger bounce
        v.bounce.reset();
        v.bounce.trigger (velocity);

        // Reset filter
        v.filter.reset();
        v.filter.setMode (CytomicSVF::Mode::LowPass);
    }

    void noteOff (int noteNumber) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.noteNumber == noteNumber && !v.releasing)
            {
                v.releasing = true;
                break;
            }
        }
    }

    uint64_t voiceCounter = 0;

    // Parameter pointers
    std::atomic<float>* pOscWave        = nullptr;
    std::atomic<float>* pOscFold        = nullptr;
    std::atomic<float>* pOscDetune      = nullptr;
    std::atomic<float>* pLevel          = nullptr;
    std::atomic<float>* pGlide          = nullptr;
    std::atomic<float>* pPercClick      = nullptr;
    std::atomic<float>* pPercDecay      = nullptr;
    std::atomic<float>* pBounceRate     = nullptr;
    std::atomic<float>* pBounceGravity  = nullptr;
    std::atomic<float>* pBounceDamp     = nullptr;
    std::atomic<float>* pBounceCnt      = nullptr;
    std::atomic<float>* pBounceSwing    = nullptr;
    std::atomic<float>* pClickTone      = nullptr;
    std::atomic<float>* pFilterCut      = nullptr;
    std::atomic<float>* pFilterRes      = nullptr;
    std::atomic<float>* pAttack         = nullptr;
    std::atomic<float>* pDecay          = nullptr;
    std::atomic<float>* pSustain        = nullptr;
    std::atomic<float>* pRelease        = nullptr;
    std::atomic<float>* pPrismDelay     = nullptr;
    std::atomic<float>* pPrismSpread    = nullptr;
    std::atomic<float>* pPrismColor     = nullptr;
    std::atomic<float>* pPrismWidth     = nullptr;
    std::atomic<float>* pPrismFeedback  = nullptr;
    std::atomic<float>* pPrismMix       = nullptr;
    std::atomic<float>* pPrismDamp      = nullptr;
    std::atomic<float>* pPhaserRate     = nullptr;
    std::atomic<float>* pPhaserDepth    = nullptr;
    std::atomic<float>* pPhaserFeedback = nullptr;
    std::atomic<float>* pPhaserMix      = nullptr;
};

} // namespace xomnibus
