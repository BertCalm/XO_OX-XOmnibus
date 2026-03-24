#pragma once
//==============================================================================
//
//  ObliqueEngine.h — XOblique: Prismatic Bounce Engine
//
//  XO_OX Designs | Accent: Prism Violet #BF40FF | Param prefix: oblq_
//
//  AQUATIC IDENTITY — THE PRISM FISH
//  XOblique is the prismatic light refracting through shallow water.
//  It lives in the sunlit shallows of the XO_OX water column — feliX-leaning,
//  bright character with depth underneath. Where sunlight hits the surface at
//  an oblique angle and shatters into spectral fragments, each bouncing between
//  coral walls like a ricocheting pinball of color.
//
//  SONIC LINEAGE
//  Run the Jewels (El-P's aggressive wavefold production) x Funk/Disco
//  (mirror-ball bounce, rhythmic clicks) x Tame Impala (psychedelic phaser
//  swirl, spectral delay). The engine channels:
//    - Buchla Music Easel: sine wavefolding as timbral control
//    - Roland Space Echo RE-201: multi-tap delay with spectral filtering
//    - Mu-Tron Bi-Phase: dual-sweep allpass phaser for psychedelic color
//    - Simmons SDS-V: tuned electronic percussion clicks (the bounce engine)
//
//  SIGNAL FLOW
//  MIDI -> Dual PolyBLEP Osc -> Wavefolder (harmonic grit) -> Cytomic SVF
//       + ObliqueBounce (per-voice percussive ricochet clicks)
//       -> Stereo Voice Mix -> ObliquePrism (6-tap spectral delay)
//       -> ObliquePhaser (6-stage allpass swirl) -> Master Output
//
//  COUPLING ROLE
//  getSampleForCoupling: post-FX stereo output + envelope follower
//  applyCouplingInput: filter cutoff, pitch, prism modulation
//  Best pairings: OBLIQUE -> OVERDUB (prismatic fragments through dub FX),
//                 ONSET -> OBLIQUE (drum hits trigger bounce cascades)
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/PitchBendUtil.h"
#include <array>
#include <cmath>
#include <vector>

namespace xolokun {

//==============================================================================
// ObliqueWavefolder — Sine wavefolder for harmonic enrichment.
//
// Technique lineage: Buchla 259 Complex Waveform Generator.
// Don Buchla pioneered wavefolding as a timbral axis — rather than filtering
// harmonics away (subtractive), you fold the waveform back on itself to ADD
// harmonics. El-P (Run the Jewels) uses similar aggressive waveshaping to give
// synth lines their signature gritty-but-musical character.
//
// At low fold: gentle saturation (warm even harmonics).
// At high fold: metallic overtones (dense odd+even spectrum).
// Two-stage fold prevents aliasing by soft-clipping the output.
//==============================================================================
class ObliqueWavefolder
{
public:
    float process (float input, float foldAmount) noexcept
    {
        // foldAmount [0, 1] maps to gain [1, 6] — each unit of gain
        // pushes the waveform further past the folding threshold,
        // creating progressively denser harmonic content
        float gain = 1.0f + foldAmount * 5.0f;  // 5.0 = max additional gain stages
        float sample = input * gain;

        // Stage 1: Sine fold — the core Buchla wavefolder technique.
        // Multiplying by pi/2 maps the [-1, +1] input range onto a full
        // sine half-cycle, so peaks fold back toward zero gracefully
        static constexpr float kHalfPi = 1.5707963f;   // pi/2
        static constexpr float kPi     = 3.14159265f;   // pi

        sample = fastSin (sample * kHalfPi);

        // Stage 2: At foldAmount > 0.3, apply a second fold pass.
        // The 0.3 threshold prevents double-folding at gentle settings
        // where it would just add CPU cost without audible benefit.
        // The extra sweep from pi/2 to 3*pi/2 creates increasingly
        // complex interference patterns — the "metallic" zone.
        if (foldAmount > 0.3f)
        {
            float secondFoldDepth = (foldAmount - 0.3f) / 0.7f;  // normalize 0.3-1.0 -> 0-1
            sample = fastSin (sample * (kHalfPi + secondFoldDepth * kPi));
        }

        return fastTanh (sample);  // soft clip prevents output exceeding [-1, +1]
    }
};

//==============================================================================
// ObliqueBounce — Bouncing-ball rhythm generator.
//
// Technique lineage: Simmons SDS-V electronic percussion + Buchla 266
// Source of Uncertainty. The bouncing-ball pattern is a staple of electronic
// music production — from 808 cowbell rolls to Aphex Twin's accelerating
// trigger bursts. Here it models light ricocheting between mirrors in a
// kaleidoscope: each reflection arrives sooner and dimmer than the last.
//
// Physics model:
//   interval[n] = initial_interval * gravity^n    (intervals shrink)
//   velocity[n] = initial_velocity * damping^n    (hits get quieter)
//   swing offsets even-numbered bounces for groove (funk DNA)
//
// The click itself is a tuned sine burst with exponential decay —
// think Simmons tom at very short decay, or a tuned impulse.
// Each note-on fires its own independent bounce sequence so
// polyphonic playing creates overlapping ricochet patterns.
//==============================================================================
class ObliqueBounce
{
public:
    void prepare (double sampleRate) noexcept
    {
        hostSampleRate = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        active = false;
        bounceIndex = 0;
        sampleCounter = 0;
        currentIntervalSamples = 0;
        clickLevel = 0.0f;
        clickPhase = 0.0f;
    }

    struct Params {
        float rate;        // Initial bounce interval in ms [20-500]
        float gravity;     // Interval shrink ratio per bounce [0.3-0.95] (lower = faster acceleration)
        float damping;     // Velocity decay per bounce [0.3-0.95] (lower = quicker fadeout)
        int   maxBounces;  // Maximum bounce count [2-16]
        float swing;       // Swing amount [0-1] — offsets odd-indexed bounces for groove
        float clickTone;   // Click sine burst center frequency in Hz [200-8000]
        // D004 fix: clickDecay controls per-click burst duration (0=1ms, 1=30ms).
        // Wired to oblq_percDecay parameter — the click envelope length is now audibly controllable.
        float clickDecay;  // Click burst duration [0-1] maps to [1ms-30ms]
    };

    void trigger (float velocity) noexcept
    {
        active = true;
        bounceIndex = 0;
        sampleCounter = 0;
        initialVelocity = velocity;
        cachedGravityPow = 1.0f;   // gravity^0 = 1 (first bounce uses full interval)
        cachedDampingPow = 1.0f;   // damping^0 = 1 (first bounce uses full velocity)
        fireClick (velocity);
    }

    // Returns click output sample in range [-1, +1]
    float process (const Params& bounceParams) noexcept
    {
        if (!active) return 0.0f;

        // D004 fix: capture clickDecay from params each call so fireClick() sees the latest value.
        currentClickDecay = bounceParams.clickDecay;

        // Advance click amplitude decay (exponential)
        clickLevel *= clickDecayCoefficient;
        // Flush denormals: decaying exponentials approach zero asymptotically,
        // producing subnormal floats that cause massive CPU stalls on x86/ARM
        // when the FPU falls back to microcode handling
        clickLevel = flushDenormal (clickLevel);
        clickPhase += clickPhaseIncrement;

        float output = clickLevel * fastSin (clickPhase);

        // Count samples toward next bounce trigger
        sampleCounter++;

        // Compute interval using cached power — avoids calling pow() every sample.
        // interval[n] = rate * gravity^n (the core physics equation)
        float intervalMs = bounceParams.rate * cachedGravityPow;

        // Apply swing to odd-indexed bounces: stretches every other interval
        // by up to 40% for a funk/disco groove feel
        static constexpr float kMaxSwingStretch = 0.4f;
        if (bounceIndex % 2 == 1)
            intervalMs *= (1.0f + bounceParams.swing * kMaxSwingStretch);

        currentIntervalSamples = static_cast<int> (intervalMs * 0.001f * static_cast<float> (hostSampleRate));
        // Minimum 4 samples between bounces to prevent audio-rate clicking
        if (currentIntervalSamples < 4) currentIntervalSamples = 4;

        if (sampleCounter >= currentIntervalSamples)
        {
            sampleCounter = 0;
            bounceIndex++;

            // Incremental power update: pow(x, n+1) = pow(x, n) * x
            // Avoids expensive std::pow() call on every bounce
            cachedGravityPow *= bounceParams.gravity;
            cachedDampingPow *= bounceParams.damping;

            // Terminate when all bounces exhausted or interval < 5ms
            // (below 5ms the bounces merge into a buzz, losing their identity)
            if (bounceIndex >= bounceParams.maxBounces || intervalMs < 5.0f)
            {
                active = false;
                return output;
            }

            // Fire next bounce click with velocity scaled by accumulated damping
            float scaledVelocity = initialVelocity * cachedDampingPow;
            static constexpr float kTwoPi = 6.28318530718f;
            clickPhaseIncrement = bounceParams.clickTone * kTwoPi / static_cast<float> (hostSampleRate);
            fireClick (scaledVelocity);
        }

        return output;
    }

    bool isActive() const noexcept { return active; }

private:
    double hostSampleRate = 44100.0;
    bool active = false;
    int bounceIndex = 0;
    int sampleCounter = 0;
    int currentIntervalSamples = 0;
    float initialVelocity = 1.0f;
    float cachedGravityPow = 1.0f;   // gravity^bounceIndex — updated incrementally each bounce
    float cachedDampingPow = 1.0f;   // damping^bounceIndex — updated incrementally each bounce

    // Click synthesis state (tuned sine burst with exponential decay)
    float clickLevel = 0.0f;
    float clickPhase = 0.0f;
    float clickPhaseIncrement = 0.0f;
    float clickDecayCoefficient = 0.99f;
    // D004 fix: currentClickDecay stores the latest Params.clickDecay for use in fireClick().
    float currentClickDecay = 0.1f;  // default ~3ms (matches legacy hardcoded value)

    void fireClick (float velocity) noexcept
    {
        clickLevel = clamp (velocity, 0.0f, 1.0f);
        clickPhase = 0.0f;
        // D004 fix: click duration now controlled by oblq_percDecay (currentClickDecay).
        // currentClickDecay [0-1] maps to click duration [1ms-30ms], giving a wide
        // range from tight snaps (1ms) to recognizable pitched transients (30ms).
        // At currentClickDecay=0.1 (default ~0.003s), matches original 3ms behaviour.
        static constexpr float kDecayTarget = 0.001f;  // -60dB silence threshold
        float clickDurationMs = 1.0f + currentClickDecay * 29.0f;  // 1ms to 30ms
        float decaySamples = std::max (1.0f, static_cast<float> (hostSampleRate) * clickDurationMs * 0.001f);
        clickDecayCoefficient = std::pow (kDecayTarget, 1.0f / decaySamples);
    }
};

//==============================================================================
// ObliquePrism — 6-facet spectral delay (the prismatic core).
//
// Technique lineage: Roland RE-201 Space Echo (multi-head tape delay) +
// Eventide H3000 (spectral processing) + disco mirror ball (6 faces of light).
//
// The sonic metaphor: a glass prism splits white light into a rainbow.
// Here, audio passes through 6 delay "facets," each filtered at a
// different frequency — splitting the sound into a spectral rainbow.
//
// Each facet provides:
//   - Delay time: staggered in golden-ratio proportions for rhythmic interest
//   - Bandpass filter: tuned to a spectral color (red=bass through violet=air)
//   - Pan position: spread across the stereo field
//   - Level: inverse-distance rolloff (closer facets are louder)
//
// Feedback between facets creates kaleidoscopic multiplication —
// sound fragments bouncing between mirrors, each reflection picking up
// a new spectral color. Like light trapped in a kaleidoscope, the
// reflections grow increasingly complex and colorful with each pass.
//
// The 6 spectral colors map audio frequency to light frequency:
//   Facet 0: Red    (100 Hz  — sub/bass warmth)
//   Facet 1: Orange (300 Hz  — body/warmth)
//   Facet 2: Yellow (800 Hz  — mid presence)
//   Facet 3: Green  (2000 Hz — upper-mid bite)
//   Facet 4: Blue   (5000 Hz — presence/air)
//   Facet 5: Violet (12000 Hz — sparkle/brilliance)
//==============================================================================
class ObliquePrism
{
public:
    static constexpr int kNumFacets = 6;
    static constexpr int kMaxDelaySamples = 96000;  // ~2 seconds at 48kHz — enough for long ambient tails

    void prepare (double sampleRate) noexcept
    {
        hostSampleRate = sampleRate;
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePosition = 0;

        // Spectral color frequencies — audio-to-light spectrum mapping.
        // Chosen to span the full audible range with musically useful spacing:
        // roughly octave-and-a-half intervals, matching how we perceive
        // spectral "color" in both light and sound.
        static constexpr float kDefaultColorFreqs[kNumFacets] = {
            100.0f, 300.0f, 800.0f, 2000.0f, 5000.0f, 12000.0f
        };

        // Default pan positions: spread evenly from hard left (0.0) to hard right (1.0)
        static constexpr float kDefaultPanPositions[kNumFacets] = {
            0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f
        };

        float sampleRateFloat = static_cast<float> (hostSampleRate);
        for (int i = 0; i < kNumFacets; ++i)
        {
            facetFilters[i].setMode (CytomicSVF::Mode::BandPass);
            facetFilters[i].setCoefficients (kDefaultColorFreqs[i], 0.4f, sampleRateFloat);
            facetFilters[i].reset();
            facetPanPositions[i] = kDefaultPanPositions[i];
        }

        // Feedback damping filter — lowpass to progressively darken
        // each feedback pass, like light losing energy with each mirror reflection
        feedbackDampingFilter.setMode (CytomicSVF::Mode::LowPass);
        feedbackDampingFilter.setCoefficients (8000.0f, 0.0f, sampleRateFloat);
        feedbackDampingFilter.reset();
    }

    void reset() noexcept
    {
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePosition = 0;
        for (int i = 0; i < kNumFacets; ++i)
            facetFilters[i].reset();
        feedbackDampingFilter.reset();
        feedbackAccumulator = 0.0f;
    }

    struct Params {
        float baseDelay;    // Base delay time in ms [10-500]
        float spread;       // Time spread between facets [0-1] (golden-ratio stagger)
        float colorSpread;  // Spectral separation of bandpass filters [0-1]
        float stereoWidth;  // Pan spread across stereo field [0-1]
        float feedback;     // Feedback amount [0-0.95] (kaleidoscopic reflections)
        float mix;          // Dry/wet mix [0-1]
        float damping;      // Feedback HF damping [0-1] (darker = more mirror absorption)
    };

    void process (float inputL, float inputR, float& outL, float& outR,
                  const Params& prismParams) noexcept
    {
        float sampleRateFloat = static_cast<float> (hostSampleRate);
        float monoInput = (inputL + inputR) * 0.5f;

        // Write input + feedback into delay buffer.
        // The feedback sample is filtered first to simulate energy loss
        // at each mirror reflection (high frequencies absorb faster).
        float dampedFeedback = feedbackDampingFilter.processSample (feedbackAccumulator);
        // Flush denormals: the feedback path is a recursive loop where
        // decaying signals inevitably produce subnormal floats that
        // cause CPU stalls when processed by the FPU
        dampedFeedback = flushDenormal (dampedFeedback);
        float writeValue = monoInput + dampedFeedback * prismParams.feedback;
        writeValue = fastTanh (writeValue);  // soft saturation prevents feedback runaway

        delayBuffer[static_cast<size_t> (writePosition)] = writeValue;

        // Update bandpass filter frequencies based on colorSpread.
        // At colorSpread=0: all 6 filters converge to 1kHz (monochrome — no spectral split)
        // At colorSpread=1: full spectrum spread (100Hz-12kHz — full rainbow)
        static constexpr float kColorCenterFreqs[kNumFacets] = {
            100.0f, 300.0f, 800.0f, 2000.0f, 5000.0f, 12000.0f
        };
        static constexpr float kConvergenceFreq = 1000.0f;  // monochrome convergence point
        // Resonance increases with color spread: narrower bands = more vivid spectral separation
        static constexpr float kBaseResonance = 0.35f;
        static constexpr float kResonanceRange = 0.3f;

        for (int i = 0; i < kNumFacets; ++i)
        {
            float targetFreq = lerp (kConvergenceFreq, kColorCenterFreqs[i], prismParams.colorSpread);
            float resonance = kBaseResonance + prismParams.colorSpread * kResonanceRange;
            facetFilters[i].setCoefficients (targetFreq, resonance, sampleRateFloat);
        }

        // Feedback damping: sweep lowpass from 16kHz (bright) to 2kHz (dark)
        static constexpr float kDampBrightFreq = 16000.0f;
        static constexpr float kDampDarkFreq = 2000.0f;
        float dampFreq = lerp (kDampBrightFreq, kDampDarkFreq, prismParams.damping);
        feedbackDampingFilter.setCoefficients (dampFreq, 0.0f, sampleRateFloat);

        // --- Read from each facet tap ---
        float wetL = 0.0f, wetR = 0.0f;
        float feedbackSum = 0.0f;

        // Facet delay ratios: golden-ratio-inspired spacing (Fibonacci-adjacent).
        // These ratios create rhythmically interesting tap patterns that avoid
        // the "evenly spaced = boring" problem. The golden ratio (phi = 1.618)
        // is nature's anti-pattern — it never repeats, never aligns, always
        // sounds organic. Each ratio is approximately phi * n.
        static constexpr float kFacetDelayRatios[kNumFacets] = {
            1.0f, 1.618f, 2.0f, 2.618f, 3.236f, 4.236f
        };

        static constexpr float kHalfPi = 1.5707963f;

        for (int i = 0; i < kNumFacets; ++i)
        {
            // Compute delay time: base delay * ratio, modulated by spread
            float delayMs = prismParams.baseDelay * lerp (1.0f, kFacetDelayRatios[i], prismParams.spread);
            int delaySamples = static_cast<int> (delayMs * 0.001f * sampleRateFloat);
            delaySamples = std::min (delaySamples, kMaxDelaySamples - 1);
            if (delaySamples < 1) delaySamples = 1;

            // Read from circular delay buffer
            int readPosition = writePosition - delaySamples;
            if (readPosition < 0) readPosition += kMaxDelaySamples;

            float tapSample = delayBuffer[static_cast<size_t> (readPosition)];

            // Apply spectral color filter to this tap
            float coloredSample = facetFilters[i].processSample (tapSample);

            // Inverse-distance level scaling: closer facets (lower index) are louder.
            // The 0.3 scaling factor gives a gentle rolloff (~2.5dB per facet).
            static constexpr float kLevelRolloffFactor = 0.3f;
            float facetLevel = 1.0f / (1.0f + static_cast<float> (i) * kLevelRolloffFactor);

            // Equal-power pan law using cos/sin (constant-power stereo placement)
            float panPosition = lerp (0.5f, facetPanPositions[i], prismParams.stereoWidth);
            float panGainL = std::cos (panPosition * kHalfPi);
            float panGainR = std::sin (panPosition * kHalfPi);

            wetL += coloredSample * facetLevel * panGainL;
            wetR += coloredSample * facetLevel * panGainR;
            feedbackSum += coloredSample * facetLevel;
        }

        // Average the feedback across all facets to prevent level buildup
        feedbackAccumulator = flushDenormal (feedbackSum / static_cast<float> (kNumFacets));

        // Advance circular buffer write position
        writePosition = (writePosition + 1) % kMaxDelaySamples;

        // Crossfade dry/wet
        outL = inputL * (1.0f - prismParams.mix) + wetL * prismParams.mix;
        outR = inputR * (1.0f - prismParams.mix) + wetR * prismParams.mix;
    }

private:
    double hostSampleRate = 44100.0;

    // Circular delay buffer (shared across all facets — single write, 6 reads)
    std::array<float, kMaxDelaySamples> delayBuffer {};
    int writePosition = 0;

    // Per-facet bandpass filters (spectral color separation)
    CytomicSVF facetFilters[kNumFacets];
    float facetPanPositions[kNumFacets] = {};

    // Feedback path
    CytomicSVF feedbackDampingFilter;
    float feedbackAccumulator = 0.0f;
};

//==============================================================================
// ObliquePhaser — 6-stage allpass phaser for psychedelic swirl.
//
// Technique lineage: Mu-Tron Bi-Phase (the phaser on Tame Impala's
// "Innerspeaker" and "Lonerism") + Electro-Harmonix Small Stone.
// Kevin Parker's guitar tone is defined by deep, slow phaser sweeps
// that create a dreamlike, underwater quality. The allpass cascade
// produces frequency-dependent phase shifts that, when mixed with
// the dry signal, create sweeping notch/peak patterns.
//
// 6 stages = 6 notches in the frequency response, creating a rich
// comb-like pattern. Feedback intensifies the notches, making the
// sweep more vocal and resonant. Applied post-prism so the spectral
// delay fragments get phase-shifted into kaleidoscopic interference.
//
// Stage spread (15% frequency offset per stage) prevents all notches
// from landing at the same frequency, which would sound thin.
// Instead, the notches fan out like fingers, creating the characteristic
// "liquid" phaser sweep.
//==============================================================================
class ObliquePhaser
{
public:
    static constexpr int kNumStages = 6;  // 6 allpass stages = 6 notches

    void prepare (double sampleRate) noexcept
    {
        hostSampleRate = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumStages; ++i)
        {
            allpassStagesL[i].reset();
            allpassStagesR[i].reset();
            allpassStagesL[i].setMode (CytomicSVF::Mode::AllPass);
            allpassStagesR[i].setMode (CytomicSVF::Mode::AllPass);
        }
        lfoPhase = 0.0;
    }

    struct Params {
        float rate;      // LFO sweep rate in Hz [0.05-8]
        float depth;     // Sweep frequency range [0-1] (wider = more dramatic)
        float feedback;  // Notch intensity [0-0.95] (higher = more resonant)
        float mix;       // Dry/wet mix [0-1]
    };

    void process (float& inOutL, float& inOutR, const Params& phaserParams) noexcept
    {
        float sampleRateFloat = static_cast<float> (hostSampleRate);

        // --- LFO ---
        // Sine LFO normalized to [0, 1] for frequency interpolation.
        // Using double precision for phase accumulator to prevent drift
        // over long performance durations (single float loses precision
        // after ~3 minutes at low LFO rates).
        static constexpr double kTwoPi = 6.28318530718;
        lfoPhase += static_cast<double> (phaserParams.rate) / hostSampleRate;
        if (lfoPhase > 1.0) lfoPhase -= 1.0;
        float lfoValue = fastSin (static_cast<float> (lfoPhase * kTwoPi));
        lfoValue = lfoValue * 0.5f + 0.5f;  // bipolar [-1,+1] -> unipolar [0,1]

        // --- Sweep frequency range ---
        // 200 Hz floor (below voice fundamental) to 4000 Hz ceiling (presence range).
        // The depth parameter controls the ceiling: at depth=0, sweep is only
        // 200-400 Hz (subtle bass wobble). At depth=1, full 200-4000 Hz sweep
        // (the classic Tame Impala dramatic phaser).
        static constexpr float kSweepFloorHz = 200.0f;
        static constexpr float kSweepCeilingMinHz = 400.0f;
        static constexpr float kSweepCeilingMaxHz = 4000.0f;

        float sweepCeiling = lerp (kSweepCeilingMinHz, kSweepCeilingMaxHz, phaserParams.depth);
        float sweepFrequency = lerp (kSweepFloorHz, sweepCeiling, lfoValue);

        // --- Set allpass frequencies with inter-stage spread ---
        // Each successive stage is 15% higher in frequency than the previous,
        // fanning the notches across the spectrum for a fuller sweep character
        static constexpr float kInterStageSpread = 0.15f;
        // Nyquist safety margin: 0.49 * sampleRate prevents filter instability
        static constexpr float kNyquistSafetyFactor = 0.49f;

        for (int i = 0; i < kNumStages; ++i)
        {
            float stageFrequency = sweepFrequency * (1.0f + static_cast<float> (i) * kInterStageSpread);
            stageFrequency = clamp (stageFrequency, 20.0f, sampleRateFloat * kNyquistSafetyFactor);
            allpassStagesL[i].setCoefficients (stageFrequency, 0.5f, sampleRateFloat);
            allpassStagesR[i].setCoefficients (stageFrequency, 0.5f, sampleRateFloat);
        }

        // --- Process through allpass cascade with feedback ---
        float wetL = inOutL + feedbackL * phaserParams.feedback;
        float wetR = inOutR + feedbackR * phaserParams.feedback;
        // Soft-clip the feedback input to prevent runaway oscillation.
        // Without this, feedback > 0.8 can cause the cascade to blow up.
        wetL = fastTanh (wetL);
        wetR = fastTanh (wetR);

        for (int i = 0; i < kNumStages; ++i)
        {
            wetL = allpassStagesL[i].processSample (wetL);
            wetR = allpassStagesR[i].processSample (wetR);
        }

        // Store feedback for next sample.
        // Flush denormals: the allpass cascade output decays toward zero
        // between sweeps, producing subnormal floats that cause CPU spikes
        feedbackL = flushDenormal (wetL);
        feedbackR = flushDenormal (wetR);

        // Crossfade dry/wet
        inOutL = inOutL * (1.0f - phaserParams.mix) + wetL * phaserParams.mix;
        inOutR = inOutR * (1.0f - phaserParams.mix) + wetR * phaserParams.mix;
    }

private:
    double hostSampleRate = 44100.0;
    double lfoPhase = 0.0;  // double precision to prevent drift over long sessions

    // Stereo allpass cascade (independent L/R for true stereo phasing)
    CytomicSVF allpassStagesL[kNumStages];
    CytomicSVF allpassStagesR[kNumStages];

    // Per-channel feedback state
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;
};

//==============================================================================
// ObliqueVoice — Per-voice state.
//
// Each voice is a "light beam" passing through the prism engine:
//   1. Dual oscillators generate the beam (sustained tone with detuned width)
//   2. Wavefolder adds harmonic grit (the RTJ character)
//   3. SVF filter shapes the spectral envelope
//   4. ADSR envelope controls amplitude
//   5. ObliqueBounce fires percussive ricochet clicks on note-on
//
// The bounce clicks ride on top of the beam like percussive fragments
// ricocheting off mirror surfaces — the disco/funk DNA of the engine.
// Each voice has its own independent bounce sequence, so polyphonic
// playing creates overlapping ricochet patterns (like multiple balls
// bouncing in a house of mirrors simultaneously).
//==============================================================================
struct ObliqueVoice
{
    // --- Voice lifecycle ---
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;          // monotonic counter for oldest-voice stealing

    // --- Oscillators (dual osc for body + stereo width) ---
    PolyBLEP oscillatorPrimary;      // Primary body tone (Saw/Square/Pulse/etc.)
    PolyBLEP oscillatorSecondary;    // Detuned secondary for chorus-like width

    // --- Per-voice filter ---
    CytomicSVF voiceFilter;          // Cytomic SVF (topology-preserving transform)

    // --- Amplitude envelope (ADSR) ---
    float envelopeLevel = 0.0f;
    float envelopeStage = 0.0f;      // 0=attack, 1=decay, 2=sustain, 3=release
    bool releasing = false;

    // --- Voice stealing crossfade ---
    float stealFadeLevel = 0.0f;     // 1.0 = fading out stolen voice, 0.0 = normal

    // --- Bounce (per-voice percussive ricochet) ---
    ObliqueBounce bounce;

    // --- Portamento/glide ---
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;

    // --- Legato detection ---
    // True when the voice was in attack/decay/sustain (gate open) at the time
    // a new note arrives. Used in Legato mode to skip envelope retrigger and
    // instead glide pitch smoothly to the new target.
    bool wasLegatoActive = false;
};

//==============================================================================
// ObliqueEngine — Prismatic Bounce Engine ("XOblique")
//
// CREATURE: The Prism Fish — a shallow-water species that refracts
// sunlight into spectral fragments as it moves. feliX-leaning: bright,
// rhythmic, percussive, but with prismatic depth underneath. Lives in
// the sunlit shallows of the XO_OX water column, between the surface
// flash of OddfeliX and the reef warmth of XOblongBob.
//
// SONIC DNA: Run the Jewels (El-P wavefold grit) x Funk/Disco (mirror-ball
// bounce, swing clicks) x Tame Impala (psychedelic phaser, spectral delay).
//
// SIGNAL ARCHITECTURE:
//   MIDI -> Dual PolyBLEP Oscillator (body + detuned width)
//        -> ObliqueWavefolder (Buchla-style harmonic enrichment)
//        -> Cytomic SVF Voice Filter (spectral shaping)
//        -> ADSR Envelope (amplitude contour)
//        + ObliqueBounce (per-voice percussive ricochet clicks)
//        -> Stereo Voice Mix (body center + bounce scattered)
//        -> ObliquePrism (6-facet spectral delay — the prismatic core)
//        -> ObliquePhaser (6-stage allpass psychedelic swirl)
//        -> Master Level -> Output
//
// Accent: Prism Violet #BF40FF
// Parameter prefix: oblq_
// Max voices: 8
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
        hostSampleRate = sampleRate;
        sampleRateFloat = static_cast<float> (hostSampleRate);

        prismDelay.prepare (hostSampleRate);
        phaserEffect.prepare (hostSampleRate);
        aftertouch.prepare (hostSampleRate);  // D006: 5ms attack / 20ms release smoothing

        couplingCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        couplingCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.oscillatorPrimary.reset();
            voice.oscillatorSecondary.reset();
            voice.voiceFilter.reset();
            voice.voiceFilter.setMode (CytomicSVF::Mode::LowPass);
            voice.bounce.prepare (hostSampleRate);
        }

        silenceGate.prepare (sampleRate, maxBlockSize);
    }

    void releaseResources() override
    {
        couplingCacheL.clear();
        couplingCacheR.clear();
    }

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.envelopeLevel = 0.0f;
            voice.stealFadeLevel = 0.0f;
            voice.releasing = false;
            voice.oscillatorPrimary.reset();
            voice.oscillatorSecondary.reset();
            voice.voiceFilter.reset();
            voice.bounce.reset();
        }
        prismDelay.reset();
        phaserEffect.reset();
        envelopeFollowerOutput = 0.0f;
        externalPitchModulation = 0.0f;
        externalFilterModulation = 0.0f;
        obliqueLfoPhase = 0.0;
        std::fill (couplingCacheL.begin(), couplingCacheL.end(), 0.0f);
        std::fill (couplingCacheR.begin(), couplingCacheR.end(), 0.0f);
    }

    //--------------------------------------------------------------------------
    // Audio rendering
    //--------------------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // ======================================================================
        // ParamSnapshot — cache all parameter values once per block.
        // This avoids atomic loads on every sample (30 params x 512 samples
        // = 15,360 atomic reads saved per block).
        // ======================================================================

        // --- Oscillator parameters ---
        const int oscWaveIndex        = (pOscWave     != nullptr) ? static_cast<int> (pOscWave->load())     : 1;
        const float oscFoldAmount     = (pOscFold     != nullptr) ? pOscFold->load()     : 0.3f;
        const float oscDetuneCents    = (pOscDetune   != nullptr) ? pOscDetune->load()   : 8.0f;
        const float masterLevel       = (pLevel       != nullptr) ? pLevel->load()       : 0.8f;
        const float glideTime         = (pGlide       != nullptr) ? pGlide->load()       : 0.0f;
        // Voice mode: 0=Poly (always retrigger), 1=Mono, 2=Legato (no retrigger when gate open).
        // Default Poly (0) preserves existing behaviour for all existing presets.
        const int   voiceMode         = (pVoiceMode   != nullptr) ? static_cast<int> (pVoiceMode->load()) : 0;

        // --- Bounce (percussive ricochet) parameters ---
        const float bounceClickLevel  = (pPercClick != nullptr) ? pPercClick->load() : 0.6f;
        const float bounceClickDecay  = (pPercDecay != nullptr) ? pPercDecay->load() : 0.3f;
        const float bounceRateMs      = (pBounceRate    != nullptr) ? pBounceRate->load()    : 80.0f;
        const float bounceGravity     = (pBounceGravity != nullptr) ? pBounceGravity->load() : 0.7f;
        const float bounceDamping     = (pBounceDamp    != nullptr) ? pBounceDamp->load()    : 0.75f;
        const int   bounceCount       = (pBounceCnt     != nullptr) ? static_cast<int> (pBounceCnt->load()) : 6;
        const float bounceSwing       = (pBounceSwing   != nullptr) ? pBounceSwing->load()   : 0.15f;
        const float bounceClickTone   = (pClickTone     != nullptr) ? pClickTone->load()     : 3000.0f;

        // --- Filter parameters ---
        const float filterCutoff      = (pFilterCut       != nullptr) ? pFilterCut->load()       : 4000.0f;
        const float filterResonance   = (pFilterRes       != nullptr) ? pFilterRes->load()       : 0.35f;
        const float filterEnvDepth    = (pFilterEnvDepth  != nullptr) ? pFilterEnvDepth->load()  : 0.3f;

        // --- Envelope parameters ---
        const float attackTime        = (pAttack    != nullptr) ? pAttack->load()    : 0.005f;
        const float decayTime         = (pDecay     != nullptr) ? pDecay->load()     : 0.3f;
        const float sustainLevel      = (pSustain   != nullptr) ? pSustain->load()   : 0.7f;
        const float releaseTime       = (pRelease   != nullptr) ? pRelease->load()   : 0.2f;

        // --- Prism delay parameters ---
        const float prismDelayMs      = (pPrismDelay    != nullptr) ? pPrismDelay->load()    : 80.0f;
        const float prismSpread       = (pPrismSpread   != nullptr) ? pPrismSpread->load()   : 0.6f;
        const float prismColorSpread  = (pPrismColor    != nullptr) ? pPrismColor->load()    : 0.7f;
        const float prismStereoWidth  = (pPrismWidth    != nullptr) ? pPrismWidth->load()    : 0.8f;
        const float prismFeedback     = (pPrismFeedback != nullptr) ? pPrismFeedback->load() : 0.4f;
        const float prismMixAmount    = (pPrismMix      != nullptr) ? pPrismMix->load()      : 0.45f;
        const float prismDamping      = (pPrismDamp     != nullptr) ? pPrismDamp->load()     : 0.3f;

        // --- Phaser parameters ---
        const float phaserRate        = (pPhaserRate     != nullptr) ? pPhaserRate->load()     : 0.5f;
        const float phaserDepth       = (pPhaserDepth    != nullptr) ? pPhaserDepth->load()    : 0.6f;
        const float phaserFeedback    = (pPhaserFeedback != nullptr) ? pPhaserFeedback->load() : 0.3f;
        const float phaserMixAmount   = (pPhaserMix      != nullptr) ? pPhaserMix->load()      : 0.4f;

        // D002: LFO1 — prism color spread modulator (now user-controllable rate/depth).
        // Default 0.2 Hz / 0.15 depth preserves all existing preset behaviour.
        const float lfo1Rate  = (pLfo1Rate  != nullptr) ? pLfo1Rate->load()  : 0.2f;
        const float lfo1Depth = (pLfo1Depth != nullptr) ? pLfo1Depth->load() : 0.15f;

        // D002: LFO2 — phaser depth modulator. Read once per block.
        const float lfo2Rate  = (pLfo2Rate  != nullptr) ? pLfo2Rate->load()  : 0.03f;
        const float lfo2Depth = (pLfo2Depth != nullptr) ? pLfo2Depth->load() : 0.25f;

        // -- XOlokun macros (CHARACTER, MOVEMENT, COUPLING, SPACE) ------------
        // Loaded once per block; defaults to 0.0 so existing presets are unaffected.
        const float macroFold   = (pMacroFold   != nullptr) ? pMacroFold->load()   : 0.0f;
        const float macroBounce = (pMacroBounce != nullptr) ? pMacroBounce->load() : 0.0f;
        const float macroColor  = (pMacroColor  != nullptr) ? pMacroColor->load()  : 0.0f;
        const float macroSpace  = (pMacroSpace  != nullptr) ? pMacroSpace->load()  : 0.0f;

        // Apply macro offsets to DSP parameters (additive, clamped to valid ranges):
        //   FOLD:   oscFoldAmount + 0.7 — harmonic grit (wavefold depth)
        //   BOUNCE: bounceRateMs + 220ms, bounceGravity + 0.2 — punchier ricochets
        //   COLOR:  prismColorSpread + 0.5, prismMixAmount + 0.35 — more spectral fragments
        //   SPACE:  phaserMixAmount + 0.5, prismFeedback + 0.3 — wider psychedelic swirl
        const float effectiveOscFold    = clamp (oscFoldAmount   + macroFold   * 0.7f,  0.0f,  1.0f);
        const float effectiveBounceRate = clamp (bounceRateMs    + macroBounce * 220.0f, 20.0f, 500.0f);
        const float effectiveBounceGrav = clamp (bounceGravity   + macroBounce * 0.2f,  0.3f,  0.95f);
        const float effectivePrismColor = clamp (prismColorSpread + macroColor * 0.5f,  0.0f,  1.0f);
        const float macroPrismMixBase   = clamp (prismMixAmount   + macroColor * 0.35f, 0.0f,  1.0f);
        const float effectivePhaserMix  = clamp (phaserMixAmount  + macroSpace * 0.5f,  0.0f,  1.0f);
        const float effectivePrismFb    = clamp (prismFeedback    + macroSpace * 0.3f,  0.0f,  0.95f);

        // ======================================================================
        // MIDI processing
        // ======================================================================

        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        oscWaveIndex, oscDetuneCents, glideTime, effectiveBounceRate, bounceClickTone,
                        voiceMode);
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            // D006: CC1 mod wheel → prism color spread (more spectral color with wheel; sensitivity 0.3)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelValue = msg.getControllerValue() / 127.0f;
            // D006: channel pressure → aftertouch (applied to prism mix depth below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);  // channel-mode: voice 0 holds global value

        // D006: aftertouch deepens prism mix (sensitivity 0.3).
        // Pressing harder pushes more signal through the 6-facet spectral delay —
        // the light bends further, more colour, more shimmer.
        // Macro-modulated base + aftertouch: both increase prism mix depth.
        const float effectivePrismMix = clamp (macroPrismMixBase + atPressure * 0.3f, 0.0f, 1.0f);

        // ======================================================================
        // Coupling input — consume accumulated external modulation
        // ======================================================================

        float couplingPitchMod = externalPitchModulation;
        float couplingFilterMod = externalFilterModulation;
        externalPitchModulation = 0.0f;
        externalFilterModulation = 0.0f;

        // Hoist filter coefficient update outside the sample loop.
        // Filter coefficients only need recalculating once per block
        // (parameter changes are block-rate, not sample-rate).
        static constexpr float kCouplingFilterModRange = 4000.0f;  // max coupling offset in Hz
        float baseCutoff = filterCutoff + couplingFilterMod * kCouplingFilterModRange;
        baseCutoff = clamp (baseCutoff, 20.0f, 20000.0f);

        // D001: per-voice filter envelope depth — velocity × envelopeLevel sweeps
        // the SVF cutoff open at the attack peak and decays with the amplitude.
        // Max additional sweep: filterEnvDepth × velocity × 7000 Hz.
        static constexpr float kFilterEnvMaxSweep = 7000.0f;
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            float envVelBoost = filterEnvDepth * voice.velocity * voice.envelopeLevel * kFilterEnvMaxSweep;
            float voiceCutoff = clamp (baseCutoff + envVelBoost, 20.0f, 20000.0f);
            voice.voiceFilter.setCoefficients (voiceCutoff, filterResonance, sampleRateFloat);
        }

        // ======================================================================
        // Per-sample rendering
        // ======================================================================

        float peakEnvelopeLevel = 0.0f;
        // Hoist block-constant detune ratio out of per-sample loop
        const float detuneRatio = fastPow2 (oscDetuneCents / 1200.0f);

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float voiceSumL = 0.0f, voiceSumR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- ADSR Envelope ---
                // Minimum time threshold: 1ms. Below this, set instantaneous
                // to avoid division-by-near-zero artifacts.
                static constexpr float kMinEnvelopeTime = 0.001f;

                if (!voice.releasing)
                {
                    if (voice.envelopeStage < 1.0f)
                    {
                        // Attack phase: linear ramp from 0 to 1
                        float attackRate = (attackTime > kMinEnvelopeTime)
                            ? 1.0f / (attackTime * sampleRateFloat) : 1.0f;
                        voice.envelopeLevel += attackRate;
                        if (voice.envelopeLevel >= 1.0f)
                        {
                            voice.envelopeLevel = 1.0f;
                            voice.envelopeStage = 1.0f;  // transition to decay
                        }
                    }
                    else if (voice.envelopeStage < 2.0f)
                    {
                        // Decay phase: exponential approach toward sustain level
                        float decayRate = (decayTime > kMinEnvelopeTime)
                            ? 1.0f / (decayTime * sampleRateFloat) : 1.0f;
                        voice.envelopeLevel -= (voice.envelopeLevel - sustainLevel) * decayRate;
                        // Flush denormals: exponential decay toward sustain
                        // produces subnormals as the difference shrinks
                        voice.envelopeLevel = flushDenormal (voice.envelopeLevel);
                        // Convergence threshold: within 0.1% of sustain = close enough
                        if (voice.envelopeLevel <= sustainLevel + 0.001f)
                        {
                            voice.envelopeLevel = sustainLevel;
                            voice.envelopeStage = 2.0f;  // transition to sustain hold
                        }
                    }
                    // Stage 2.0+: sustain — hold at sustainLevel until note-off
                }
                else
                {
                    // Release phase: exponential decay toward zero
                    float releaseRate = (releaseTime > kMinEnvelopeTime)
                        ? 1.0f / (releaseTime * sampleRateFloat) : 1.0f;
                    voice.envelopeLevel -= voice.envelopeLevel * releaseRate;
                    // Flush denormals: release decay is the most common source
                    // of subnormals — every released note decays toward zero
                    voice.envelopeLevel = flushDenormal (voice.envelopeLevel);
                    // -60dB silence threshold: below this, the voice is inaudible
                    static constexpr float kSilenceThreshold = 0.001f;
                    if (voice.envelopeLevel < kSilenceThreshold)
                    {
                        voice.envelopeLevel = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // --- Voice stealing crossfade ---
                // When a voice is stolen, it fades out over 5ms to prevent clicks.
                // 5ms is the standard click-free crossfade duration for voice stealing.
                float stealFadeGain = 1.0f;
                static constexpr float kStealFadeDuration = 0.005f;  // 5ms crossfade
                if (voice.stealFadeLevel > 0.0f)
                {
                    voice.stealFadeLevel -= 1.0f / (kStealFadeDuration * sampleRateFloat);
                    if (voice.stealFadeLevel <= 0.0f) voice.stealFadeLevel = 0.0f;
                    stealFadeGain = 1.0f - voice.stealFadeLevel;
                }

                // --- Portamento/Glide ---
                if (glideTime > kMinEnvelopeTime)
                {
                    // Glide rate: half the glide time gives a musically useful
                    // exponential approach speed (reaches target in ~2x glide time)
                    float glideRate = 1.0f / (glideTime * sampleRateFloat * 0.5f);
                    float frequencyDelta = voice.targetFrequency - voice.currentFrequency;
                    voice.currentFrequency += frequencyDelta * glideRate;
                }
                else
                {
                    voice.currentFrequency = voice.targetFrequency;
                }
                // Apply coupling pitch modulation (in semitones, via pow2) + pitch bend
                float frequency = voice.currentFrequency * fastPow2 (couplingPitchMod)
                                  * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);

                // --- Dual Oscillator ---
                // Primary oscillator at pitch; secondary detuned by oscDetuneCents
                // for chorus-like stereo width. Detune is in cents (1/100 semitone),
                // converted to frequency ratio via 2^(cents/1200) — hoisted to block level.
                voice.oscillatorPrimary.setFrequency (frequency, sampleRateFloat);
                voice.oscillatorSecondary.setFrequency (
                    frequency * detuneRatio, sampleRateFloat);

                // Mix ratio: 70/30 primary/secondary. The primary carries the
                // fundamental weight; the secondary adds shimmer without muddying.
                static constexpr float kPrimaryMixLevel = 0.7f;
                static constexpr float kSecondaryMixLevel = 0.3f;
                float oscillatorOutput = voice.oscillatorPrimary.processSample() * kPrimaryMixLevel
                                       + voice.oscillatorSecondary.processSample() * kSecondaryMixLevel;

                // --- Wavefolder (Buchla-inspired harmonic grit) ---
                // D001 enhancement: velocity scales fold depth.
                // Harder hits push the waveform through more folding stages,
                // adding harmonic complexity. At velocity=1.0: +0.25 fold boost.
                // At velocity=0.2 (soft): +0.05 — gentle warmth, no grit.
                // This maps directly to the prism-fish character: hard throws of
                // the ball scatter more spectral fragments; soft ones glow steadily.
                static constexpr float kVelocityFoldBoost = 0.25f;
                float velocityFoldAmount = clamp (effectiveOscFold + voice.velocity * kVelocityFoldBoost, 0.0f, 1.0f);
                oscillatorOutput = wavefolder.process (oscillatorOutput, velocityFoldAmount);

                // --- Voice filter (Cytomic SVF lowpass) ---
                oscillatorOutput = voice.voiceFilter.processSample (oscillatorOutput);

                // --- Apply envelope + velocity + steal fade ---
                float voicedOutput = oscillatorOutput * voice.envelopeLevel
                                   * voice.velocity * stealFadeGain;

                // --- Bounce clicks (per-voice percussive ricochets) ---
                ObliqueBounce::Params bounceParams;
                bounceParams.rate = effectiveBounceRate;
                bounceParams.gravity = effectiveBounceGrav;
                bounceParams.damping = bounceDamping;
                bounceParams.maxBounces = bounceCount;
                bounceParams.swing = bounceSwing;
                bounceParams.clickTone = bounceClickTone;
                // D004 fix: bounceClickDecay (oblq_percDecay) now controls click burst duration.
                // Normalize: param range is 0.001–0.05, remap to 0–1 for the Bounce's
                // clickDecay field (which maps 0–1 → 1ms–30ms click duration).
                bounceParams.clickDecay = clamp ((bounceClickDecay - 0.001f) / (0.05f - 0.001f), 0.0f, 1.0f);

                float bounceOutput = voice.bounce.process (bounceParams)
                                   * bounceClickLevel * voice.velocity;

                // --- Stereo voice mix ---
                // Body signal panned center (equal L/R at 70% level).
                // Bounce clicks get per-voice pan scatter based on note number
                // for spatial interest — each note's ricochets land at a different
                // stereo position, like reflections from different mirror facets.
                static constexpr float kBodyPanLevel = 0.7f;
                float bodyL = voicedOutput * kBodyPanLevel;
                float bodyR = voicedOutput * kBodyPanLevel;

                // Pan scatter: map note number (mod 7) to 0.3-0.7 pan range.
                // Using mod 7 (a prime) ensures adjacent notes don't cluster.
                static constexpr float kBouncePanBase = 0.3f;
                static constexpr float kBouncePanRange = 0.4f;
                static constexpr float kHalfPi = 1.5707963f;  // pi/2 for equal-power pan law
                float bouncePan = kBouncePanBase
                    + (static_cast<float> (voice.noteNumber % 7) / 7.0f) * kBouncePanRange;
                float bounceL = bounceOutput * std::cos (bouncePan * kHalfPi);
                float bounceR = bounceOutput * std::sin (bouncePan * kHalfPi);

                voiceSumL += bodyL + bounceL;
                voiceSumR += bodyR + bounceR;

                if (voice.envelopeLevel > peakEnvelopeLevel)
                    peakEnvelopeLevel = voice.envelopeLevel;
            }

            // ================================================================
            // Post-voice FX chain
            // ================================================================

            // --- D002/D005: Prism LFO (LFO1) — user-controllable rate and depth ---
            // Rate: lfo1Rate (user param, 0.005–4 Hz) + COLOR macro boost (up to +2 Hz).
            // Depth: lfo1Depth (user param, 0–0.5) + BOUNCE macro deepening (up to +0.20).
            // Default lfo1Rate=0.2, lfo1Depth=0.15 preserves all existing preset behaviour.
            // Double precision phase prevents drift over long performance sessions.
            static constexpr double kLfo1MacroRateBoost = 2.0;   // Hz added at full COLOR macro
            static constexpr double kTwoPiD  = 6.28318530718;
            double effectiveLfoRate = static_cast<double> (lfo1Rate)
                                    + static_cast<double> (macroColor) * kLfo1MacroRateBoost;
            obliqueLfoPhase += effectiveLfoRate / hostSampleRate;
            if (obliqueLfoPhase >= 1.0) obliqueLfoPhase -= 1.0;
            float obliqueLfoValue = fastSin (static_cast<float> (obliqueLfoPhase * kTwoPiD));
            // lfo1Depth + BOUNCE macro deepening. The prism fish catches a rotating light beam.
            float lfoDepth = lfo1Depth + macroBounce * 0.20f;
            float lfoColorMod = obliqueLfoValue * lfoDepth;

            // --- D002: 2nd LFO — phaser swirl modulator ---
            // User-controlled rate (0.005–2 Hz) and depth (0–0.5).
            // Modulates phaser depth, creating autonomous breathing of the Tame Impala swirl.
            // The phase floor at 0.005 Hz satisfies D005 (≤ 0.01 Hz rate floor).
            obliqueLfo2Phase += static_cast<double> (lfo2Rate) / hostSampleRate;
            if (obliqueLfo2Phase >= 1.0) obliqueLfo2Phase -= 1.0;
            float lfo2Value = fastSin (static_cast<float> (obliqueLfo2Phase * kTwoPiD));
            // Unipolar LFO2 [0, 1] for phaser depth: never goes to zero, adds on top of base
            float lfo2PhaserMod = lfo2Value * lfo2Depth;

            // --- Prism Delay (6-facet spectral delay — the prismatic core) ---
            // D006: mod wheel increases prism color spread up to +0.3 at full wheel (sensitivity 0.3)
            // D005: prism LFO adds ±0.15 autonomous spectral breathing
            ObliquePrism::Params prismParams;
            prismParams.baseDelay = prismDelayMs;
            prismParams.spread = prismSpread;
            prismParams.colorSpread = clamp (effectivePrismColor + modWheelValue * 0.3f + lfoColorMod, 0.0f, 1.0f);
            prismParams.stereoWidth = prismStereoWidth;
            prismParams.feedback = effectivePrismFb;
            prismParams.mix = effectivePrismMix;
            prismParams.damping = prismDamping;

            float postPrismL = 0.0f, postPrismR = 0.0f;
            prismDelay.process (voiceSumL, voiceSumR, postPrismL, postPrismR, prismParams);

            // --- Phaser (Tame Impala psychedelic swirl) ---
            ObliquePhaser::Params phaserParams;
            phaserParams.rate = phaserRate;
            // D002: LFO2 modulates phaser depth — bipolar, adds to base depth.
            // At lfo2Depth=0.25 and full LFO swing: phaser depth breathes ±0.25.
            // Clamped so it never goes below 0 or above 1.
            phaserParams.depth = clamp (phaserDepth + lfo2PhaserMod, 0.0f, 1.0f);
            phaserParams.feedback = phaserFeedback;
            phaserParams.mix = effectivePhaserMix;
            phaserEffect.process (postPrismL, postPrismR, phaserParams);

            // --- Master output ---
            float outputL = postPrismL * masterLevel;
            float outputR = postPrismR * masterLevel;

            // Store in coupling cache for getSampleForCoupling() reads
            auto cacheIndex = static_cast<size_t> (sampleIndex);
            if (cacheIndex < couplingCacheL.size())
            {
                couplingCacheL[cacheIndex] = outputL;
                couplingCacheR[cacheIndex] = outputR;
            }

            // Write to output buffer (addSample for mixing with other engines)
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sampleIndex, outputL);
                buffer.addSample (1, sampleIndex, outputR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sampleIndex, (outputL + outputR) * 0.5f);
            }
        }

        envelopeFollowerOutput = peakEnvelopeLevel;

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
    }

    //--------------------------------------------------------------------------
    // Coupling — XO_OX cross-engine modulation interface.
    //
    // Channel 0: Left output (post-FX)
    // Channel 1: Right output (post-FX)
    // Channel 2: Envelope follower (peak envelope level from last block)
    //
    // Best coupling pairings for OBLIQUE:
    //   OBLIQUE -> OVERDUB: prismatic fragments through dub FX chain
    //   ONSET -> OBLIQUE: drum transients trigger bounce cascades
    //   OBLIQUE -> ODYSSEY: prism output modulates JOURNEY macro
    //   OBLONG -> OBLIQUE: warm coral textures refracted through prism
    //--------------------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto index = static_cast<size_t> (sampleIndex);
        if (channel == 0 && index < couplingCacheL.size()) return couplingCacheL[index];
        if (channel == 1 && index < couplingCacheR.size()) return couplingCacheR[index];
        if (channel == 2) return envelopeFollowerOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                // Direct filter cutoff modulation — classic ducking/pumping
                externalFilterModulation += amount;
                break;

            case CouplingType::AmpToPitch:
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                // Pitch modulation scaled by 0.5 to keep it musical
                // (full range would be too extreme for melodic content)
                externalPitchModulation += amount * 0.5f;
                break;

            case CouplingType::EnvToDecay:
                // External envelope modulates filter cutoff at 30% strength.
                // This creates a subtle breathing effect where external
                // transients "open" the prism's spectral window.
                externalFilterModulation += amount * 0.3f;
                break;

            case CouplingType::RhythmToBlend:
                // Rhythm coupling modulates filter at 20% — lighter touch
                // for groove-locked spectral movement
                externalFilterModulation += amount * 0.2f;
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

        // Round 10F: Voice mode for legato detection.
        // 0=Poly (always retrigger — default, existing behaviour preserved),
        // 1=Mono (retrigger on every new note, single voice),
        // 2=Legato (new note while gate open: slide pitch, skip retrigger + bounce).
        // This is the `wasActive` check that was missing from the Round 9D audit.
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "oblq_voiceMode", 1 }, "Oblique Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));

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

        // D001: Filter envelope depth — velocity × ADSR envelope sweeps the SVF
        // cutoff open at the attack peak, then falls back toward the base cutoff
        // as the note decays. This makes harder velocity hits brighter and gives
        // Oblique's prismatic lines D001-compliant timbre response.
        // Default 0.3: audible brightness sweep without overwhelming the base tone.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_filterEnvDepth", 1 }, "Oblique Filter Env Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

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

        // D002: LFO1 user rate — prism color spread modulator.
        // Previously hardcoded at 0.2 Hz. Now user-controllable: slow glacial sweep
        // to fast prismatic flutter. Color macro scales rate toward 2.0 Hz on top of
        // the base rate set here. Default 0.2 Hz preserves all existing preset behaviour.
        // Rate floor 0.005 Hz satisfies D005 (≤ 0.01 Hz LFO breathing requirement).
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_lfo1Rate", 1 }, "Oblique LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 4.0f, 0.005f, 0.3f), 0.2f));

        // D002: LFO1 depth — prism color spread modulation depth.
        // Default 0.15 preserves existing behaviour (±0.15 spectral spread).
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_lfo1Depth", 1 }, "Oblique LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 0.5f, 0.01f), 0.15f));

        // D002: 2nd LFO — phaser swirl LFO. Modulates phaser depth for autonomous
        // Tame Impala-style breathing movement. Rate floor 0.005 Hz satisfies D005.
        // Default 0.03 Hz = one full phaser depth sweep every ~33 seconds (very slow,
        // like light gradually shifting angle through the prism).
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_lfo2Rate", 1 }, "Oblique LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 2.0f, 0.005f, 0.3f), 0.03f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_lfo2Depth", 1 }, "Oblique LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 0.5f, 0.01f), 0.25f));

        // XOlokun standard macros (CHARACTER, MOVEMENT, COUPLING, SPACE)
        // All default to 0.0 — existing presets are unaffected.
        //
        // M1 FOLD (CHARACTER): increases wavefold amount by up to +0.7 — harmonic grit.
        //   At max: oscFold + 0.7 (clamped to 1.0) — the prism fish refracts hard.
        // M2 BOUNCE (MOVEMENT): increases bounce rate by up to +220ms and gravity by +0.2.
        //   At max: bounceRateMs + 220ms, bounceGravity + 0.2 — faster, punchier ricochets.
        // M3 COLOR (COUPLING): increases prism color spread +0.5 and prism mix +0.35.
        //   At max: prismColor + 0.5, prismMix + 0.35 — spectral fragments multiply.
        // M4 SPACE (SPACE): increases phaser mix +0.5 and prism feedback +0.3.
        //   At max: phaserMix + 0.5, prismFeedback + 0.3 — wide psychedelic swirl.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_macroFold",   1 }, "Oblique FOLD",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_macroBounce", 1 }, "Oblique BOUNCE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_macroColor",  1 }, "Oblique COLOR",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "oblq_macroSpace",  1 }, "Oblique SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscWave        = apvts.getRawParameterValue ("oblq_oscWave");
        pOscFold        = apvts.getRawParameterValue ("oblq_oscFold");
        pOscDetune      = apvts.getRawParameterValue ("oblq_oscDetune");
        pLevel          = apvts.getRawParameterValue ("oblq_level");
        pGlide          = apvts.getRawParameterValue ("oblq_glide");
        pVoiceMode      = apvts.getRawParameterValue ("oblq_voiceMode");
        pPercClick      = apvts.getRawParameterValue ("oblq_percClick");
        pPercDecay      = apvts.getRawParameterValue ("oblq_percDecay");
        pBounceRate     = apvts.getRawParameterValue ("oblq_bounceRate");
        pBounceGravity  = apvts.getRawParameterValue ("oblq_bounceGravity");
        pBounceDamp     = apvts.getRawParameterValue ("oblq_bounceDamp");
        pBounceCnt      = apvts.getRawParameterValue ("oblq_bounceCnt");
        pBounceSwing    = apvts.getRawParameterValue ("oblq_bounceSwing");
        pClickTone      = apvts.getRawParameterValue ("oblq_clickTone");
        pFilterCut        = apvts.getRawParameterValue ("oblq_filterCut");
        pFilterRes        = apvts.getRawParameterValue ("oblq_filterRes");
        pFilterEnvDepth   = apvts.getRawParameterValue ("oblq_filterEnvDepth");
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
        // D002: LFO1 (prism color) and LFO2 (phaser swirl)
        pLfo1Rate       = apvts.getRawParameterValue ("oblq_lfo1Rate");
        pLfo1Depth      = apvts.getRawParameterValue ("oblq_lfo1Depth");
        pLfo2Rate       = apvts.getRawParameterValue ("oblq_lfo2Rate");
        pLfo2Depth      = apvts.getRawParameterValue ("oblq_lfo2Depth");
        // XOlokun macros
        pMacroFold      = apvts.getRawParameterValue ("oblq_macroFold");
        pMacroBounce    = apvts.getRawParameterValue ("oblq_macroBounce");
        pMacroColor     = apvts.getRawParameterValue ("oblq_macroColor");
        pMacroSpace     = apvts.getRawParameterValue ("oblq_macroSpace");
    }

    //--------------------------------------------------------------------------
    // Identity — The Prism Fish
    // Sunlit shallows dweller, accent Prism Violet #BF40FF
    //--------------------------------------------------------------------------

    juce::String getEngineId() const override { return "Oblique"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFBF40FF); }  // Prism Violet
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active) ++count;
        return count;
    }

private:

    SilenceGate silenceGate;

    //--------------------------------------------------------------------------
    // Core state
    //--------------------------------------------------------------------------

    double hostSampleRate = 44100.0;
    float sampleRateFloat = 44100.0f;

    //--------------------------------------------------------------------------
    // Voice pool — 8-voice polyphony with oldest-note stealing
    //--------------------------------------------------------------------------

    std::array<ObliqueVoice, kMaxVoices> voices {};

    //--------------------------------------------------------------------------
    // Shared DSP modules (post-voice, applied to the summed voice output)
    //--------------------------------------------------------------------------

    ObliqueWavefolder wavefolder;     // Per-voice, but shared instance (stateless)
    ObliquePrism prismDelay;          // 6-facet spectral delay (the prismatic core)
    ObliquePhaser phaserEffect;       // 6-stage allpass psychedelic swirl

    // D006: mod wheel — CC1 increases prism color spread (more spectral color with wheel; sensitivity 0.3)
    float modWheelValue  = 0.0f;
    float pitchBendNorm  = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // D005: Prism LFO — autonomous spectral modulation (D005 compliance).
    // A slow sine LFO at 0.2 Hz modulates prism color spread ±0.15, making the
    // spectral rainbow breathe and shift without any user interaction. This is
    // the "prism fish catching the light" — sunlight angle always changing.
    // Double precision phase accumulator prevents drift over long sessions.
    // Rate 0.2 Hz = one full spectral sweep every 5 seconds. Depth ±0.15 is
    // perceptible but not distracting — the sound lives and moves.
    double obliqueLfoPhase = 0.0;

    // D002: 2nd LFO — phaser swirl LFO (D002 requires 2+ LFOs).
    // Rate: user-controlled via oblq_lfo2Rate (0.005–2.0 Hz, floor ≤ 0.01 Hz for D005).
    // Destination: phaser depth modulation (±lfo2Depth, making the Tame Impala swirl breathe).
    // This is the second axis of autonomous modulation: LFO1 colors the prism, LFO2 animates the phaser.
    double obliqueLfo2Phase = 0.0;

    //--------------------------------------------------------------------------
    // Coupling state — accumulated between blocks, consumed in renderBlock
    //--------------------------------------------------------------------------

    float envelopeFollowerOutput = 0.0f;      // peak envelope for coupling channel 2
    float externalPitchModulation = 0.0f;     // accumulated pitch mod from other engines
    float externalFilterModulation = 0.0f;    // accumulated filter mod from other engines

    // D006: CS-80-inspired poly aftertouch (channel pressure → prism mix depth)
    PolyAftertouch aftertouch;

    //--------------------------------------------------------------------------
    // Output cache — stores per-sample output for getSampleForCoupling() reads.
    // Other engines read from this cache during the coupling phase.
    //--------------------------------------------------------------------------

    std::vector<float> couplingCacheL;
    std::vector<float> couplingCacheR;

    //--------------------------------------------------------------------------
    // MIDI handlers
    //--------------------------------------------------------------------------

    void noteOn (int noteNumber, float velocity, int oscWaveIndex, float detuneCents,
                 float glideTime, float bounceRateMs, float bounceClickTone,
                 int voiceMode) noexcept
    {
        // -----------------------------------------------------------------------
        // Legato detection (voiceMode == 2)
        //
        // In Legato mode, if ANY voice is currently gate-open (active and not
        // releasing) we treat this as a legato note — slide its pitch to the
        // new target without retriggering the envelope or the bounce sequence.
        // This is the same "wasActive" check used by Origami, Oracle, and Morph.
        //
        // In Poly (0) or Mono (1) mode (or when no gate-open voice exists),
        // fall through to normal voice allocation below.
        // -----------------------------------------------------------------------
        static constexpr float kMinGlideTime = 0.001f;
        static constexpr float kMinFrequencyForGlide = 10.0f;

        if (voiceMode == 2)  // Legato
        {
            for (int i = 0; i < kMaxVoices; ++i)
            {
                auto& v = voices[static_cast<size_t> (i)];
                // Gate is open: active voice that has NOT yet entered release
                if (v.active && !v.releasing)
                {
                    // Slide pitch to new target — keep the running envelope intact
                    float noteFrequency = midiToFreq (noteNumber);
                    v.noteNumber = noteNumber;
                    v.velocity   = velocity;
                    v.targetFrequency = noteFrequency;
                    // If no glide time, snap immediately to prevent pitch jump
                    if (glideTime <= kMinGlideTime)
                        v.currentFrequency = noteFrequency;
                    v.wasLegatoActive = true;
                    return;  // legato: no retrigger, no bounce, no new voice
                }
            }
        }

        // -----------------------------------------------------------------------
        // Normal (Poly / Mono) voice allocation: find free voice or steal oldest
        // -----------------------------------------------------------------------
        int selectedVoiceIndex = -1;
        uint64_t oldestStartTime = UINT64_MAX;
        int oldestVoiceIndex = 0;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[static_cast<size_t> (i)].active)
            {
                selectedVoiceIndex = i;
                break;
            }
            if (voices[static_cast<size_t> (i)].startTime < oldestStartTime)
            {
                oldestStartTime = voices[static_cast<size_t> (i)].startTime;
                oldestVoiceIndex = i;
            }
        }

        // If no free voice found, steal the oldest and crossfade it out
        if (selectedVoiceIndex < 0)
        {
            selectedVoiceIndex = oldestVoiceIndex;
            voices[static_cast<size_t> (selectedVoiceIndex)].stealFadeLevel = 1.0f;
        }

        auto& voice = voices[static_cast<size_t> (selectedVoiceIndex)];

        // --- Capture legato glide intent BEFORE resetting voice state ---
        // wasAlreadyActive: true only when this voice was previously playing
        // (either still sustaining or being stolen). Only in this case do we
        // allow portamento — connecting the outgoing note's pitch to the new one.
        // If the voice was idle (freshly allocated), the pitch jumps immediately
        // even when glide > 0, preventing unintended portamento in staccato play.
        // This mirrors the Origami/Morph pattern from Round 11D.
        const bool wasAlreadyActive = voice.active && voice.currentFrequency > kMinFrequencyForGlide;

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = ++voiceCounter;
        voice.releasing = false;
        voice.envelopeStage = 0.0f;
        voice.envelopeLevel = 0.0f;
        voice.wasLegatoActive = false;

        // --- Set oscillator waveform ---
        PolyBLEP::Waveform waveform;
        switch (oscWaveIndex)
        {
            case 0:  waveform = PolyBLEP::Waveform::Sine;     break;
            case 1:  waveform = PolyBLEP::Waveform::Saw;      break;
            case 2:  waveform = PolyBLEP::Waveform::Square;   break;
            case 3:  waveform = PolyBLEP::Waveform::Pulse;    break;
            case 4:  waveform = PolyBLEP::Waveform::Triangle; break;
            default: waveform = PolyBLEP::Waveform::Saw;      break;
        }
        voice.oscillatorPrimary.setWaveform (waveform);
        voice.oscillatorSecondary.setWaveform (waveform);

        // --- Set frequency (with conditional portamento glide) ---
        // Glide only applies when a voice was already playing (wasAlreadyActive).
        // Freshly allocated idle voices always jump immediately, so staccato
        // playing never produces unintended portamento — only connected notes do.
        float noteFrequency = midiToFreq (noteNumber);
        if (wasAlreadyActive && glideTime > kMinGlideTime)
            voice.targetFrequency = noteFrequency;
        else
        {
            voice.currentFrequency = noteFrequency;
            voice.targetFrequency = noteFrequency;
        }

        // --- Trigger bounce (percussive ricochet on note-on) ---
        voice.bounce.reset();
        voice.bounce.trigger (velocity);

        // --- Reset voice filter ---
        voice.voiceFilter.reset();
        voice.voiceFilter.setMode (CytomicSVF::Mode::LowPass);
    }

    void noteOff (int noteNumber) noexcept
    {
        // Release the first active, non-releasing voice matching this note
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.releasing)
            {
                voice.releasing = true;
                break;
            }
        }
    }

    //--------------------------------------------------------------------------
    // Voice allocation counter (monotonically increasing for oldest-note steal)
    //--------------------------------------------------------------------------

    uint64_t voiceCounter = 0;

    //--------------------------------------------------------------------------
    // Parameter pointers — cached from APVTS in attachParameters().
    // Grouped by function to match the ParamSnapshot layout in renderBlock.
    //--------------------------------------------------------------------------

    // Oscillator
    std::atomic<float>* pOscWave        = nullptr;
    std::atomic<float>* pOscFold        = nullptr;
    std::atomic<float>* pOscDetune      = nullptr;
    std::atomic<float>* pLevel          = nullptr;
    std::atomic<float>* pGlide          = nullptr;
    // Round 10F: voice mode (0=Poly, 1=Mono, 2=Legato)
    std::atomic<float>* pVoiceMode      = nullptr;

    // Bounce (percussive ricochet)
    std::atomic<float>* pPercClick      = nullptr;
    std::atomic<float>* pPercDecay      = nullptr;
    std::atomic<float>* pBounceRate     = nullptr;
    std::atomic<float>* pBounceGravity  = nullptr;
    std::atomic<float>* pBounceDamp     = nullptr;
    std::atomic<float>* pBounceCnt      = nullptr;
    std::atomic<float>* pBounceSwing    = nullptr;
    std::atomic<float>* pClickTone      = nullptr;

    // Filter
    std::atomic<float>* pFilterCut        = nullptr;
    std::atomic<float>* pFilterRes        = nullptr;
    std::atomic<float>* pFilterEnvDepth   = nullptr;

    // Envelope (ADSR)
    std::atomic<float>* pAttack         = nullptr;
    std::atomic<float>* pDecay          = nullptr;
    std::atomic<float>* pSustain        = nullptr;
    std::atomic<float>* pRelease        = nullptr;

    // Prism delay (6-facet spectral)
    std::atomic<float>* pPrismDelay     = nullptr;
    std::atomic<float>* pPrismSpread    = nullptr;
    std::atomic<float>* pPrismColor     = nullptr;
    std::atomic<float>* pPrismWidth     = nullptr;
    std::atomic<float>* pPrismFeedback  = nullptr;
    std::atomic<float>* pPrismMix       = nullptr;
    std::atomic<float>* pPrismDamp      = nullptr;

    // Phaser (psychedelic swirl)
    std::atomic<float>* pPhaserRate     = nullptr;
    std::atomic<float>* pPhaserDepth    = nullptr;
    std::atomic<float>* pPhaserFeedback = nullptr;
    std::atomic<float>* pPhaserMix      = nullptr;
    // D002: LFO1 (prism color modulator — now user-controllable)
    std::atomic<float>* pLfo1Rate       = nullptr;
    std::atomic<float>* pLfo1Depth      = nullptr;
    // D002: LFO2 (phaser swirl modulator)
    std::atomic<float>* pLfo2Rate       = nullptr;
    std::atomic<float>* pLfo2Depth      = nullptr;
    // XOlokun macros
    std::atomic<float>* pMacroFold      = nullptr;
    std::atomic<float>* pMacroBounce    = nullptr;
    std::atomic<float>* pMacroColor     = nullptr;
    std::atomic<float>* pMacroSpace     = nullptr;
};

} // namespace xolokun
