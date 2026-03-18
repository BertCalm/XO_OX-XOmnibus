#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <complex>
#include <vector>

namespace xomnibus {

//==============================================================================
//
//  ORIGAMI ENGINE -- Spectral Folding FFT Synthesis
//  XO_OX Designs | Accent: Vermillion Fold #E63946
//
//  ---- Creature Identity ----
//  ORIGAMI is paper boats folding on the current -- surface tension made
//  audible. It lives in the Sunlit Shallows of the XO_OX water column,
//  feliX-leaning: bright character with depth underneath. Where feliX's
//  electric surface energy meets the spectral patience of Oscar's deep,
//  sound folds back on itself like paper creased by water.
//
//  Fourth-generation species in the XO_OX lineage. Born from the same
//  primordial coupling as all engines, ORIGAMI adapted to the shallow-
//  water niche where light refracts through moving surfaces -- a zone
//  shared with XOblique (prismatic bounces) and XOptic (bioluminescent
//  flashes). Its unique adaptation: folding the frequency spectrum itself.
//
//  ---- Historical Synth Lineage ----
//  ORIGAMI channels the legacy of spectral processing synthesizers:
//    - Metasynth (1999) -- visual spectral editing as composition
//    - Kyma (Symbolic Sound) -- arbitrary spectral transformations
//    - Spectral Processing in Csound (pvsanal/pvsynth) -- STFT as DSP
//    - Ircam AudioSculpt -- spectral surgery on living sound
//    - Paulstretch -- time-stretching via frozen spectral frames
//  Unlike those tools (which are offline or academic), ORIGAMI performs
//  spectral folding in real time as a playable instrument, with per-voice
//  STFT pipelines running at audio rate.
//
//  ---- Technical Architecture ----
//  Sound enters the frequency domain via Short-Time Fourier Transform
//  (STFT), where 4 cascaded spectral operations -- FOLD, MIRROR, ROTATE,
//  STRETCH -- sculpt the magnitude spectrum before resynthesis. The result
//  is a class of timbres impossible through time-domain methods: spectra
//  that fold back on themselves, creating dense metallic textures,
//  crystalline harmonics, and otherworldly inharmonic tones.
//
//  Signal chain per voice:
//    Oscillator Bank (saw + square + noise mix) / Coupling Input
//      -> STFT Analysis (2048-point, 4x overlap, Hann window)
//      -> Spectral Operations (FOLD / MIRROR / ROTATE / STRETCH)
//      -> Phase Vocoder (instantaneous frequency tracking)
//      -> IFFT + Overlap-Add Resynthesis
//      -> Post-Filter (Cytomic SVF lowpass, smoothing)
//      -> Amp Envelope * Velocity * Crossfade
//      -> Stereo Pan (deterministic per voice index)
//      -> Master Level
//
//  Features:
//    - Internal oscillator bank (saw + square + noise mix) + coupling input
//    - 2048-point STFT with 4x overlap (hop = 512), Hann window
//    - 4 spectral operations: FOLD, MIRROR, ROTATE, STRETCH
//    - Phase vocoder with instantaneous frequency tracking
//    - Spectral freeze (hold current frame indefinitely)
//    - 3-point triangular smoothing on folded magnitude
//    - Amp and Fold ADSR envelopes
//    - 2 LFOs (Sine / Triangle / Saw / Square / S&H)
//    - Mono / Legato / Poly4 / Poly8 voice modes with LRU stealing + 5ms crossfade
//    - Full XOmnibus coupling support
//
//  Coupling:
//    - Output: post-fold stereo audio via getSampleForCoupling
//    - Input: AudioToWavetable (replace/blend source),
//             AmpToFilter (amplitude -> fold depth),
//             EnvToMorph (envelope -> fold point),
//             RhythmToBlend (rhythm -> freeze trigger)
//
//==============================================================================


//==============================================================================
// FFT Constants
//
// The FFT size of 2048 provides ~23ms analysis windows at 44.1kHz -- a good
// balance between frequency resolution (~21.5 Hz per bin) and temporal
// responsiveness. The 4x overlap (hop size = 512) ensures smooth spectral
// transitions with the Hann window's overlap-add reconstruction property.
//==============================================================================
static constexpr int kFFTSize     = 2048;                   // FFT frame length in samples
static constexpr int kFFTHalf     = kFFTSize / 2 + 1;      // 1025 positive-frequency bins (DC through Nyquist)
static constexpr int kHopSize     = kFFTSize / 4;           // 512 samples between analysis frames (4x overlap)
static constexpr int kOverlapFactor = 4;                    // Overlap factor: 4x for smooth Hann-window reconstruction


//==============================================================================
// OrigamiADSR -- Lightweight ADSR Envelope Generator
//
// Allocation-free, inline envelope suitable for real-time audio threads.
// Uses exponential-approximation decay curves (multiply-accumulate on the
// difference from target) for natural-sounding release tails.
//
// The 0.0001f epsilon values throughout serve as convergence thresholds --
// they prevent the envelope from asymptotically approaching its target
// forever (which would waste CPU) while remaining inaudible at -80dB.
//==============================================================================
struct OrigamiADSR
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Stage::Idle;
    float level = 0.0f;

    // Per-sample increment/coefficient for each stage
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainLevel = 1.0f;
    float releaseRate = 0.0f;

    void setParams (float attackSeconds, float decaySeconds, float sustain,
                    float releaseSeconds, float sampleRate) noexcept
    {
        float safeSampleRate = std::max (1.0f, sampleRate);

        // Minimum stage time of 1ms prevents division by zero and ensures
        // at least ~44 samples of transition at 44.1kHz
        static constexpr float kMinStageSeconds = 0.001f;

        attackRate  = (attackSeconds  > kMinStageSeconds) ? (1.0f / (attackSeconds  * safeSampleRate)) : 1.0f;
        decayRate   = (decaySeconds   > kMinStageSeconds) ? (1.0f / (decaySeconds   * safeSampleRate)) : 1.0f;
        sustainLevel = sustain;
        releaseRate = (releaseSeconds > kMinStageSeconds) ? (1.0f / (releaseSeconds * safeSampleRate)) : 1.0f;
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
        // Convergence epsilon: ~-80dB below unity, inaudible but prevents
        // infinite asymptotic decay that would waste CPU cycles
        static constexpr float kConvergenceEpsilon = 0.0001f;

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
                // Exponential-approximation curve: multiply rate by distance
                // from sustain target for natural logarithmic decay shape
                level -= decayRate * (level - sustainLevel + kConvergenceEpsilon);
                if (level <= sustainLevel + kConvergenceEpsilon)
                {
                    level = sustainLevel;
                    stage = Stage::Sustain;
                }
                return level;

            case Stage::Sustain:
                return level;

            case Stage::Release:
                // Same exponential-approximation curve toward zero
                level -= releaseRate * (level + kConvergenceEpsilon);
                if (level <= kConvergenceEpsilon)
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
// OrigamiLFO -- Multi-Shape Low-Frequency Oscillator
//
// Provides 5 waveshapes for modulation: Sine, Triangle, Saw, Square, and
// Sample-and-Hold (S&H). The S&H mode uses a Linear Congruential Generator
// (LCG) PRNG for deterministic pseudo-random values that trigger on each
// new LFO cycle, producing classic stepped random modulation.
//==============================================================================
struct OrigamiLFO
{
    enum class Shape { Sine, Triangle, Saw, Square, SampleAndHold };

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    Shape shape = Shape::Sine;
    float sampleAndHoldValue = 0.0f;

    // LCG PRNG state for Sample-and-Hold mode.
    // Initial seed 12345 is arbitrary but deterministic -- ensures identical
    // S&H sequences across instances for reproducible patch behavior.
    uint32_t prngState = 12345u;

    void setRate (float frequencyHz, float sampleRate) noexcept
    {
        phaseIncrement = frequencyHz / std::max (1.0f, sampleRate);
    }

    void setShape (int shapeIndex) noexcept
    {
        shape = static_cast<Shape> (std::min (4, std::max (0, shapeIndex)));
    }

    float process() noexcept
    {
        float output = 0.0f;

        switch (shape)
        {
            case Shape::Sine:
                // Full-cycle sine: phase [0,1] mapped to [0, 2*PI]
                output = fastSin (phase * 6.28318530718f);
                break;

            case Shape::Triangle:
                // Triangle from absolute-value fold: maps [0,1] phase to
                // [-1,+1] output with linear slopes
                output = 4.0f * std::fabs (phase - 0.5f) - 1.0f;
                break;

            case Shape::Saw:
                // Rising sawtooth: phase [0,1] mapped to [-1,+1]
                output = 2.0f * phase - 1.0f;
                break;

            case Shape::Square:
                // 50% duty cycle square wave
                output = (phase < 0.5f) ? 1.0f : -1.0f;
                break;

            case Shape::SampleAndHold:
            {
                // Trigger a new random value at each cycle boundary.
                // LCG constants from Numerical Recipes (Knuth):
                //   multiplier = 1664525, increment = 1013904223
                // These produce a full-period sequence over 2^32 values.
                float previousPhase = phase - phaseIncrement;
                if (previousPhase < 0.0f || phase < previousPhase)
                {
                    prngState = prngState * 1664525u + 1013904223u;
                    // Extract 16 bits and map to [-1, +1] bipolar range
                    sampleAndHoldValue = static_cast<float> (prngState & 0xFFFF) / 32768.0f - 1.0f;
                }
                output = sampleAndHoldValue;
                break;
            }
        }

        // Advance and wrap phase
        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= 1.0f;

        return output;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        sampleAndHoldValue = 0.0f;
        prngState = 12345u;
    }
};


//==============================================================================
// SpectralFrame -- Holds Magnitude/Phase Data for One STFT Analysis Frame
//
// Each frame stores the full positive-frequency spectrum (DC through Nyquist)
// plus the previous hop's phase data for the phase vocoder's instantaneous
// frequency estimation.
//==============================================================================
struct SpectralFrame
{
    std::array<float, kFFTHalf> magnitude {};       // Magnitude per bin (linear scale)
    std::array<float, kFFTHalf> phase {};            // Phase per bin (radians, [-PI, PI])
    std::array<float, kFFTHalf> previousPhase {};    // Phase from the previous hop (for phase vocoder delta)
    std::array<float, kFFTHalf> instantaneousFreq {};// True frequency per bin (Hz), accounting for phase deviation

    void clear() noexcept
    {
        magnitude.fill (0.0f);
        phase.fill (0.0f);
        previousPhase.fill (0.0f);
        instantaneousFreq.fill (0.0f);
    }
};


//==============================================================================
// OrigamiVoice -- Per-Voice State with Complete STFT Pipeline
//
// Each voice maintains its own circular input buffer, overlap-add output
// accumulator, spectral analysis/frozen frames, and all working buffers
// for the FFT/IFFT process. This allows true polyphonic spectral folding
// with independent spectral states per note.
//
// Memory layout: ~100KB per voice (8 voices = ~800KB total). All arrays
// are fixed-size to avoid audio-thread allocation.
//==============================================================================
struct OrigamiVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;                     // Voice allocation timestamp for LRU stealing

    // ---- Oscillator State ----
    float sawPhase = 0.0f;                       // Sawtooth phase accumulator [0, 1)
    float squarePhase = 0.0f;                    // Square wave phase accumulator [0, 1)

    // LCG PRNG state for noise oscillator.
    // Seed 12345 matches LFO S&H seed for consistency.
    uint32_t noiseGeneratorState = 12345u;

    // ---- Portamento / Glide ----
    float currentFrequency = 440.0f;             // Instantaneous frequency (Hz), smoothed toward target
    float targetFrequency = 440.0f;              // Destination frequency for glide
    float glideCoefficient = 1.0f;               // Per-sample interpolation rate (1.0 = instant)

    // ---- Envelopes ----
    OrigamiADSR ampEnvelope;                     // Controls voice amplitude
    OrigamiADSR foldEnvelope;                    // Modulates fold depth over note duration

    // ---- LFOs (per-voice for free-running independence) ----
    OrigamiLFO lfo1;                             // Primary LFO: default target is fold point
    OrigamiLFO lfo2;                             // Secondary LFO: default target is rotate amount

    // ---- STFT Analysis / Resynthesis Buffers ----
    std::array<float, kFFTSize> inputRingBuffer {};      // Circular buffer capturing oscillator output
    std::array<float, kFFTSize> overlapAddAccumulator {}; // Overlap-add output reconstruction buffer
    int inputWritePosition = 0;                           // Current write head in the circular input buffer
    int hopSampleCounter = 0;                             // Samples remaining until next STFT hop

    // ---- Spectral Frames ----
    SpectralFrame analysisFrame;                 // Most recent live analysis result
    SpectralFrame frozenFrame;                   // Snapshot held during spectral freeze
    bool hasFrozenFrame = false;                 // True once at least one frame has been captured for freeze

    // ---- STFT Working Buffers (pre-allocated, reused each hop) ----
    std::array<float, kFFTSize> windowedInput {};        // Input after Hann window multiplication
    std::array<float, kFFTSize> fftReal {};              // Real part for forward FFT
    std::array<float, kFFTSize> fftImaginary {};         // Imaginary part for forward FFT
    std::array<float, kFFTHalf> foldedMagnitude {};      // Magnitude after spectral operations
    std::array<float, kFFTHalf> foldedPhase {};          // Phase after spectral operations
    std::array<float, kFFTSize> ifftReal {};             // Real part for inverse FFT
    std::array<float, kFFTSize> ifftImaginary {};        // Imaginary part for inverse FFT
    std::array<float, kFFTSize> resynthesisWindow {};    // Windowed output for overlap-add

    // ---- Post-FFT Smoothing Filter ----
    CytomicSVF postFilter;                       // Lowpass at ~18kHz to tame FFT artifacts

    // ---- Voice Stealing Crossfade ----
    float crossfadeGain = 1.0f;                  // Gain ramp during steal transition
    bool isFadingOut = false;                    // True when this voice is being stolen

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        sawPhase = 0.0f;
        squarePhase = 0.0f;
        noiseGeneratorState = 12345u;
        currentFrequency = 440.0f;
        targetFrequency = 440.0f;
        crossfadeGain = 1.0f;
        isFadingOut = false;
        inputWritePosition = 0;
        hopSampleCounter = 0;
        hasFrozenFrame = false;
        ampEnvelope.reset();
        foldEnvelope.reset();
        lfo1.reset();
        lfo2.reset();
        postFilter.reset();
        inputRingBuffer.fill (0.0f);
        overlapAddAccumulator.fill (0.0f);
        analysisFrame.clear();
        frozenFrame.clear();
    }
};


//==============================================================================
//
//  OrigamiEngine -- The Main Engine Class
//
//  Spectral folding synthesizer implementing the XOmnibus SynthEngine
//  interface. Sound is generated by an internal oscillator bank, transformed
//  through a per-voice STFT pipeline with 4 spectral operations, then
//  resynthesized via overlap-add.
//
//  The name "Origami" reflects the core metaphor: just as paper folding
//  creates complex 3D forms from flat sheets, spectral folding creates
//  complex timbres from simple source waveforms by creasing, mirroring,
//  rotating, and stretching the frequency spectrum.
//
//==============================================================================
class OrigamiEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    // ---- Mathematical Constants ----
    static constexpr float kPI    = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    // Magnitude floor: values below this are treated as zero.
    // 1e-15 is well above the IEEE 754 float denormal threshold (~1.4e-45)
    // but far below any audible signal level (~-300dB). This protects all
    // feedback paths and filter states from denormal numbers, which cause
    // massive CPU spikes on x86 when the FPU enters microcode-assisted
    // denormal handling mode. Every magnitude comparison in the spectral
    // pipeline clamps against this floor.
    static constexpr float kMagnitudeFloor = 1e-15f;

    //==========================================================================
    //  SynthEngine Interface -- Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        storedSampleRate = sampleRate;
        sampleRateFloat = static_cast<float> (storedSampleRate);

        // Smoothing coefficient for control-rate parameters.
        // Derived from a 5ms time constant (200Hz cutoff), which prevents
        // audible zipper noise on knob movements while remaining responsive
        // enough for performance gestures.
        parameterSmoothingCoefficient = 1.0f - std::exp (-kTwoPi * (1.0f / 0.005f) / sampleRateFloat);

        // Voice-stealing crossfade rate: 5ms linear ramp.
        // 5ms is short enough to be imperceptible as a click but long enough
        // to prevent discontinuity artifacts when a voice is stolen.
        voiceCrossfadeRate = 1.0f / (0.005f * sampleRateFloat);

        // Frequency resolution: the spacing in Hz between adjacent FFT bins.
        // At 44100 Hz with 2048-point FFT, this is ~21.53 Hz per bin.
        // Used by the phase vocoder to convert bin indices to frequencies.
        frequencyPerBin = sampleRateFloat / static_cast<float> (kFFTSize);

        // P0-03 fix: guard against host block sizes smaller than the FFT hop size.
        // The STFT pipeline requires at least kHopSize (512) samples of history
        // per block to trigger a valid analysis frame. Clamp to the minimum.
        int safeBlockSize = std::max (maxBlockSize, kHopSize);

        // Allocate output cache for inter-engine coupling reads
        outputCacheLeft.resize (static_cast<size_t> (safeBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (safeBlockSize), 0.0f);

        aftertouch.prepare (sampleRate);

        silenceGate.prepare (sampleRate, maxBlockSize);
        silenceGate.setHoldTime (500.0f);  // Origami has reverb tails

        // Coupling input buffer for receiving external audio
        couplingInputBuffer.resize (static_cast<size_t> (safeBlockSize), 0.0f);

        // Build the Hann (raised-cosine) window.
        // The Hann window is chosen because its overlap-add property with 4x
        // overlap produces constant-sum reconstruction (COLA condition), which
        // is essential for artifact-free STFT resynthesis.
        for (int i = 0; i < kFFTSize; ++i)
        {
            hannWindow[static_cast<size_t> (i)] =
                0.5f * (1.0f - std::cos (kTwoPi * static_cast<float> (i) / static_cast<float> (kFFTSize)));
        }

        // Build bit-reversal table and twiddle factors for the radix-2 FFT
        buildBitReversalTable();

        // Initialize all voices to clean state
        for (auto& voice : voices)
        {
            voice.reset();
            voice.postFilter.reset();
            voice.postFilter.setMode (CytomicSVF::Mode::LowPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
            voice.reset();

        // ---- Reset envelope output ----
        envelopeOutput = 0.0f;

        // ---- Reset coupling accumulators ----
        couplingFoldDepthModulation = 0.0f;
        couplingFoldPointModulation = 0.0f;
        couplingFreezeTrigger = 0.0f;
        couplingSourceModulation = 0.0f;

        // ---- Reset smoothed parameter states ----
        smoothedFoldPoint = 0.5f;
        smoothedFoldDepth = 0.5f;
        smoothedRotate = 0.0f;
        smoothedStretch = 0.0f;

        // ---- Clear output caches ----
        std::fill (outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
        std::fill (outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
        std::fill (couplingInputBuffer.begin(), couplingInputBuffer.end(), 0.0f);
    }

    //==========================================================================
    //  SynthEngine Interface -- Audio Rendering
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // ---- ParamSnapshot: cache all parameter pointers once per block ----
        // This pattern reads atomic parameter values exactly once at block
        // boundaries, avoiding per-sample atomic loads and ensuring all
        // voices in a block see consistent parameter values.

        // Core spectral fold parameters
        const float paramFoldPoint        = loadParam (pFoldPoint, 0.5f);
        const float paramFoldDepth        = loadParam (pFoldDepth, 0.5f);
        const int   paramFoldCount        = static_cast<int> (loadParam (pFoldCount, 1.0f));
        const int   paramOperation        = static_cast<int> (loadParam (pOperation, 0.0f));
        const float paramRotate           = loadParam (pRotate, 0.0f);
        const float paramStretch          = loadParam (pStretch, 0.0f);
        const float paramFreeze           = loadParam (pFreeze, 0.0f);
        const float paramSource           = loadParam (pSource, 0.0f);
        const float paramOscillatorMix    = loadParam (pOscMix, 0.5f);
        const float paramMasterLevel      = loadParam (pLevel, 0.8f);

        // Amplitude envelope ADSR
        const float paramAmpAttack        = loadParam (pAmpAttack, 0.01f);
        const float paramAmpDecay         = loadParam (pAmpDecay, 0.1f);
        const float paramAmpSustain       = loadParam (pAmpSustain, 0.8f);
        const float paramAmpRelease       = loadParam (pAmpRelease, 0.3f);

        // Fold envelope ADSR
        const float paramFoldEnvAttack    = loadParam (pFoldEnvAttack, 0.01f);
        const float paramFoldEnvDecay     = loadParam (pFoldEnvDecay, 0.2f);
        const float paramFoldEnvSustain   = loadParam (pFoldEnvSustain, 0.5f);
        const float paramFoldEnvRelease   = loadParam (pFoldEnvRelease, 0.3f);

        // LFO parameters
        const float paramLfo1Rate         = loadParam (pLfo1Rate, 1.0f);
        const float paramLfo1Depth        = loadParam (pLfo1Depth, 0.0f);
        const int   paramLfo1Shape        = static_cast<int> (loadParam (pLfo1Shape, 0.0f));
        const float paramLfo2Rate         = loadParam (pLfo2Rate, 1.0f);
        const float paramLfo2Depth        = loadParam (pLfo2Depth, 0.0f);
        const int   paramLfo2Shape        = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        // Voice mode and glide
        const int   voiceModeIndex        = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime             = loadParam (pGlide, 0.0f);

        // Macros: M1=FOLD, M2=MOTION, M3=COUPLING, M4=SPACE
        const float macroFold             = loadParam (pMacroFold, 0.0f);
        const float macroMotion           = loadParam (pMacroMotion, 0.0f);
        const float macroCoupling         = loadParam (pMacroCoupling, 0.0f);
        const float macroSpace            = loadParam (pMacroSpace, 0.0f);

        // ---- Determine voice mode and polyphony ----
        int maxPolyphony = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIndex)
        {
            case 0: maxPolyphony = 1; monoMode = true; break;                          // Mono
            case 1: maxPolyphony = 1; monoMode = true; legatoMode = true; break;       // Legato
            case 2: maxPolyphony = 4; break;                                            // Poly4
            case 3: maxPolyphony = 8; break;                                            // Poly8
            default: maxPolyphony = 4; break;
        }

        // ---- Glide coefficient ----
        // Exponential smoothing coefficient derived from glide time.
        // At 0.001s minimum, glide is essentially instant.
        float glideCoefficient = 1.0f;
        if (glideTime > 0.001f)
            glideCoefficient = 1.0f - std::exp (-1.0f / (glideTime * sampleRateFloat));

        // ---- Apply macro and coupling modulation offsets ----

        // M1 FOLD: adds 40% of macro range to fold point, 30% to fold depth.
        // These scaling factors are tuned so that macro at 100% produces a
        // dramatic but not extreme spectral transformation.
        // D006: aftertouch added below — atPressure resolved after MIDI loop
        float effectiveFoldPoint = clamp (paramFoldPoint + macroFold * 0.4f + couplingFoldPointModulation, 0.0f, 1.0f);
        float effectiveFoldDepth = clamp (paramFoldDepth + macroFold * 0.3f + couplingFoldDepthModulation, 0.0f, 1.0f);

        // M2 MOTION: adds 50% of macro range to rotate, 30% to LFO1 depth.
        // Higher motion macro = more spectral movement and modulation depth.
        float effectiveRotate    = clamp (paramRotate + macroMotion * 0.5f, -1.0f, 1.0f);
        float lfo1EffectiveDepth = paramLfo1Depth + macroMotion * 0.3f;

        // M3 COUPLING: adds 50% of macro range to source blend.
        // Full coupling macro = half external source, still blended with internal.
        float effectiveSource    = clamp (paramSource + macroCoupling * 0.5f + couplingSourceModulation, 0.0f, 1.0f);

        // M4 SPACE: adds up to 2 extra fold cascades, 40% to stretch.
        // More space = more spectral folding passes + frequency warping.
        int effectiveFoldCount   = std::max (1, std::min (4, paramFoldCount + static_cast<int> (macroSpace * 2.0f)));
        float effectiveStretch   = clamp (paramStretch + macroSpace * 0.4f, -1.0f, 1.0f);

        // ---- Freeze: combine parameter toggle + coupling rhythm trigger ----
        bool freezeActive = (paramFreeze > 0.5f) || (couplingFreezeTrigger > 0.5f);

        // ---- Reset coupling accumulators for next block ----
        couplingFoldDepthModulation = 0.0f;
        couplingFoldPointModulation = 0.0f;
        couplingFreezeTrigger = 0.0f;
        couplingSourceModulation = 0.0f;

        // ---- Process MIDI events ----
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                silenceGate.wake();
                handleNoteOn (message.getNoteNumber(), message.getFloatVelocity(),
                              maxPolyphony, monoMode, legatoMode, glideCoefficient,
                              paramAmpAttack, paramAmpDecay, paramAmpSustain, paramAmpRelease,
                              paramFoldEnvAttack, paramFoldEnvDecay, paramFoldEnvSustain, paramFoldEnvRelease,
                              paramLfo1Rate, paramLfo1Depth, paramLfo1Shape,
                              paramLfo2Rate, paramLfo2Depth, paramLfo2Shape);
            }
            else if (message.isNoteOff())
                handleNoteOff (message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff())
                reset();
            // D006: channel pressure → aftertouch (applied to fold depth below)
            else if (message.isChannelPressure())
                aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f);
            // D006: CC1 mod wheel → STFT fold depth (more spectral processing with wheel; sensitivity 0.3)
            else if (message.isController() && message.getControllerNumber() == 1)
                modWheelValue = message.getControllerValue() / 127.0f;
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        // D006: smooth aftertouch and apply to fold depth (more spectral shimmer on pressure)
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);
        // Sensitivity 0.3: full pressure adds up to +0.3 fold depth (denser spectral folding)
        // D006: mod wheel also adds up to +0.3 fold depth (sensitivity 0.3)
        effectiveFoldDepth = clamp (effectiveFoldDepth + atPressure * 0.3f + modWheelValue * 0.3f, 0.0f, 1.0f);

        float peakEnvelopeLevel = 0.0f;

        // ---- Per-Sample Render Loop ----
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            // Smooth control-rate parameters toward their targets (5ms time constant)
            smoothedFoldPoint += (effectiveFoldPoint - smoothedFoldPoint) * parameterSmoothingCoefficient;
            smoothedFoldDepth += (effectiveFoldDepth - smoothedFoldDepth) * parameterSmoothingCoefficient;
            smoothedRotate    += (effectiveRotate    - smoothedRotate)    * parameterSmoothingCoefficient;
            smoothedStretch   += (effectiveStretch   - smoothedStretch)   * parameterSmoothingCoefficient;

            float stereoMixLeft = 0.0f, stereoMixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // ---- Voice-stealing crossfade (5ms linear ramp to zero) ----
                if (voice.isFadingOut)
                {
                    voice.crossfadeGain -= voiceCrossfadeRate;
                    if (voice.crossfadeGain <= 0.0f)
                    {
                        voice.crossfadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // ---- Portamento (exponential glide toward target frequency) ----
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency) * voice.glideCoefficient;

                // ---- Process envelopes ----
                float amplitudeLevel = voice.ampEnvelope.process();
                float foldEnvelopeLevel = voice.foldEnvelope.process();

                if (!voice.ampEnvelope.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // ---- LFO modulation ----
                float lfo1Value = voice.lfo1.process() * lfo1EffectiveDepth;
                float lfo2Value = voice.lfo2.process() * paramLfo2Depth;

                // LFO routing: LFO1 -> fold point, LFO2 -> rotate amount.
                // 0.3f scaling factor keeps modulation musical -- full LFO depth
                // shifts the fold point or rotation by +-30% of its range.
                float modulatedFoldPoint = clamp (smoothedFoldPoint + lfo1Value * 0.3f, 0.0f, 1.0f);
                float modulatedRotate    = clamp (smoothedRotate + lfo2Value * 0.3f, -1.0f, 1.0f);

                // Fold depth scaled by fold envelope (envelope controls intensity over time)
                float modulatedFoldDepth = smoothedFoldDepth * foldEnvelopeLevel;

                // ---- Generate source signal ----
                float frequency = voice.currentFrequency;
                float phaseIncrement = frequency / sampleRateFloat;

                // Sawtooth oscillator (naive -- anti-aliasing is handled by the
                // FFT process itself, which naturally bandlimits to Nyquist)
                voice.sawPhase += phaseIncrement;
                if (voice.sawPhase >= 1.0f) voice.sawPhase -= 1.0f;
                float sawSample = 2.0f * voice.sawPhase - 1.0f;

                // Square oscillator (50% duty cycle)
                voice.squarePhase += phaseIncrement;
                if (voice.squarePhase >= 1.0f) voice.squarePhase -= 1.0f;
                float squareSample = (voice.squarePhase < 0.5f) ? 1.0f : -1.0f;

                // Noise oscillator (Linear Congruential Generator).
                // Same Numerical Recipes LCG constants as the LFO S&H mode.
                voice.noiseGeneratorState = voice.noiseGeneratorState * 1664525u + 1013904223u;
                float noiseSample = static_cast<float> (voice.noiseGeneratorState & 0xFFFF) / 32768.0f - 1.0f;

                // Oscillator crossfade: oscMix maps [0, 0.5] = saw->square,
                // [0.5, 1.0] = square->noise. This creates a continuous
                // timbral morphing axis from harmonic (saw) through hollow
                // (square) to noisy (white noise).
                float oscillatorSample;
                if (paramOscillatorMix <= 0.5f)
                {
                    float crossfade = paramOscillatorMix * 2.0f;
                    oscillatorSample = sawSample * (1.0f - crossfade) + squareSample * crossfade;
                }
                else
                {
                    float crossfade = (paramOscillatorMix - 0.5f) * 2.0f;
                    oscillatorSample = squareSample * (1.0f - crossfade) + noiseSample * crossfade;
                }

                // Blend internal oscillator with external coupling input
                float couplingInput = 0.0f;
                if (sampleIndex < static_cast<int> (couplingInputBuffer.size()))
                    couplingInput = couplingInputBuffer[static_cast<size_t> (sampleIndex)];

                float sourceSample = oscillatorSample * (1.0f - effectiveSource) + couplingInput * effectiveSource;

                // ---- Feed source into STFT input ring buffer ----
                voice.inputRingBuffer[static_cast<size_t> (voice.inputWritePosition)] = sourceSample;
                voice.inputWritePosition = (voice.inputWritePosition + 1) % kFFTSize;

                // ---- Advance hop counter and trigger STFT when a full hop is ready ----
                voice.hopSampleCounter++;
                if (voice.hopSampleCounter >= kHopSize)
                {
                    voice.hopSampleCounter = 0;
                    processSTFT (voice, modulatedFoldPoint, modulatedFoldDepth, effectiveFoldCount,
                                 paramOperation, modulatedRotate, smoothedStretch, freezeActive);
                }

                // ---- Read from overlap-add output buffer ----
                int readPosition = (voice.inputWritePosition) % kFFTSize;
                float outputSample = voice.overlapAddAccumulator[static_cast<size_t> (readPosition)];
                // Clear consumed sample from the accumulator to prevent re-reading
                voice.overlapAddAccumulator[static_cast<size_t> (readPosition)] = 0.0f;

                // Denormal protection: flush sub-normal floats to zero.
                // Without this, recursive accumulation in the overlap-add buffer
                // can produce denormals that cause 100x CPU spikes on x86.
                outputSample = flushDenormal (outputSample);

                // ---- Apply amplitude envelope, velocity, and crossfade gain ----
                float voiceGain = amplitudeLevel * voice.velocity * voice.crossfadeGain;
                outputSample *= voiceGain;

                // ---- Deterministic stereo spread based on voice index ----
                // Voices are distributed across a 0.3-0.7 pan range (not hard L/R)
                // for a natural stereo image without holes in the center.
                float panPosition = 0.5f;
                if (kMaxVoices > 1)
                {
                    int voiceIndex = static_cast<int> (&voice - &voices[0]);
                    panPosition = 0.3f + 0.4f * static_cast<float> (voiceIndex) / static_cast<float> (kMaxVoices - 1);
                }
                // Constant-power panning using sin/cos law
                float panGainLeft  = std::cos (panPosition * kPI * 0.5f);
                float panGainRight = std::sin (panPosition * kPI * 0.5f);

                stereoMixLeft  += outputSample * panGainLeft;
                stereoMixRight += outputSample * panGainRight;

                peakEnvelopeLevel = std::max (peakEnvelopeLevel, amplitudeLevel);
            }

            // ---- Apply master level ----
            float finalLeft  = stereoMixLeft * paramMasterLevel;
            float finalRight = stereoMixRight * paramMasterLevel;

            // Final denormal flush on master output to protect downstream
            // processing (other engines, DAW mixer) from denormal contamination
            finalLeft  = flushDenormal (finalLeft);
            finalRight = flushDenormal (finalRight);

            // ---- Write to output buffer ----
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sampleIndex, finalLeft);
                buffer.addSample (1, sampleIndex, finalRight);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sampleIndex, (finalLeft + finalRight) * 0.5f);
            }

            // ---- Cache output for inter-engine coupling reads ----
            if (sampleIndex < static_cast<int> (outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t> (sampleIndex)]  = finalLeft;
                outputCacheRight[static_cast<size_t> (sampleIndex)] = finalRight;
            }
        }

        envelopeOutput = peakEnvelopeLevel;

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);

        // Clear coupling input buffer for next block
        std::fill (couplingInputBuffer.begin(),
                   couplingInputBuffer.begin() + std::min (static_cast<size_t> (numSamples),
                                                            couplingInputBuffer.size()),
                   0.0f);

        // Update active voice count (atomic-safe for UI thread reads)
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active) ++count;
        activeVoiceCount = count;
    }

    //==========================================================================
    //  SynthEngine Interface -- Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto index = static_cast<size_t> (sampleIndex);
        if (channel == 0 && index < outputCacheLeft.size())  return outputCacheLeft[index];
        if (channel == 1 && index < outputCacheRight.size()) return outputCacheRight[index];
        if (channel == 2) return envelopeOutput;    // Channel 2 = envelope follower for amplitude coupling
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToWavetable:
                // External audio replaces/blends with internal oscillator source.
                // The audio is written into the coupling input buffer, and the
                // source parameter controls the wet/dry blend.
                if (sourceBuffer != nullptr)
                {
                    int count = std::min (numSamples, static_cast<int> (couplingInputBuffer.size()));
                    for (int i = 0; i < count; ++i)
                        couplingInputBuffer[static_cast<size_t> (i)] += sourceBuffer[i] * amount;
                }
                // 0.3f scaling: coupling amount at 100% shifts source blend by 30%,
                // preventing total override of internal oscillator
                couplingSourceModulation += amount * 0.3f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude envelope modulates fold depth.
                // 0.5f scaling: full coupling adds 50% fold depth offset
                couplingFoldDepthModulation += amount * 0.5f;
                break;

            case CouplingType::EnvToMorph:
                // External envelope modulates fold point position.
                // 0.4f scaling: full coupling shifts fold point by 40%
                couplingFoldPointModulation += amount * 0.4f;
                break;

            case CouplingType::RhythmToBlend:
                // Rhythm pattern triggers spectral freeze.
                // Binary threshold: any coupling above 0.5 activates freeze,
                // making it work well with gate/trigger sources.
                if (amount > 0.5f)
                    couplingFreezeTrigger = 1.0f;
                break;

            default:
                break;
        }
    }

    //==========================================================================
    //  SynthEngine Interface -- Parameters
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
        // ---- Core Spectral Fold Parameters ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_foldPoint", 1 }, "Origami Fold Point",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_foldDepth", 1 }, "Origami Fold Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_foldCount", 1 }, "Origami Fold Count",
            juce::StringArray { "1", "2", "3", "4" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_operation", 1 }, "Origami Operation",
            juce::StringArray { "Fold", "Mirror", "Rotate", "Stretch" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_rotate", 1 }, "Origami Rotate",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_stretch", 1 }, "Origami Stretch",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_freeze", 1 }, "Origami Freeze",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_source", 1 }, "Origami Source",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_oscMix", 1 }, "Origami Osc Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        // ---- Master Level ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_level", 1 }, "Origami Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // ---- Amplitude Envelope ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_ampAttack", 1 }, "Origami Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_ampDecay", 1 }, "Origami Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_ampSustain", 1 }, "Origami Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_ampRelease", 1 }, "Origami Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // ---- Fold Envelope ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_foldEnvAttack", 1 }, "Origami Fold Env Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_foldEnvDecay", 1 }, "Origami Fold Env Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_foldEnvSustain", 1 }, "Origami Fold Env Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_foldEnvRelease", 1 }, "Origami Fold Env Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // ---- LFO 1 ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo1Rate", 1 }, "Origami LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo1Depth", 1 }, "Origami LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_lfo1Shape", 1 }, "Origami LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // ---- LFO 2 ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo2Rate", 1 }, "Origami LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo2Depth", 1 }, "Origami LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_lfo2Shape", 1 }, "Origami LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // ---- Voice Parameters ----

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_voiceMode", 1 }, "Origami Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_glide", 1 }, "Origami Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // ---- Macros ----

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_macroFold", 1 }, "Origami Macro FOLD",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_macroMotion", 1 }, "Origami Macro MOTION",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_macroCoupling", 1 }, "Origami Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_macroSpace", 1 }, "Origami Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        // ---- Core Spectral Parameters ----
        pFoldPoint         = apvts.getRawParameterValue ("origami_foldPoint");
        pFoldDepth         = apvts.getRawParameterValue ("origami_foldDepth");
        pFoldCount         = apvts.getRawParameterValue ("origami_foldCount");
        pOperation         = apvts.getRawParameterValue ("origami_operation");
        pRotate            = apvts.getRawParameterValue ("origami_rotate");
        pStretch           = apvts.getRawParameterValue ("origami_stretch");
        pFreeze            = apvts.getRawParameterValue ("origami_freeze");
        pSource            = apvts.getRawParameterValue ("origami_source");
        pOscMix            = apvts.getRawParameterValue ("origami_oscMix");
        pLevel             = apvts.getRawParameterValue ("origami_level");

        // ---- Amplitude Envelope ----
        pAmpAttack         = apvts.getRawParameterValue ("origami_ampAttack");
        pAmpDecay          = apvts.getRawParameterValue ("origami_ampDecay");
        pAmpSustain        = apvts.getRawParameterValue ("origami_ampSustain");
        pAmpRelease        = apvts.getRawParameterValue ("origami_ampRelease");

        // ---- Fold Envelope ----
        pFoldEnvAttack     = apvts.getRawParameterValue ("origami_foldEnvAttack");
        pFoldEnvDecay      = apvts.getRawParameterValue ("origami_foldEnvDecay");
        pFoldEnvSustain    = apvts.getRawParameterValue ("origami_foldEnvSustain");
        pFoldEnvRelease    = apvts.getRawParameterValue ("origami_foldEnvRelease");

        // ---- LFOs ----
        pLfo1Rate          = apvts.getRawParameterValue ("origami_lfo1Rate");
        pLfo1Depth         = apvts.getRawParameterValue ("origami_lfo1Depth");
        pLfo1Shape         = apvts.getRawParameterValue ("origami_lfo1Shape");
        pLfo2Rate          = apvts.getRawParameterValue ("origami_lfo2Rate");
        pLfo2Depth         = apvts.getRawParameterValue ("origami_lfo2Depth");
        pLfo2Shape         = apvts.getRawParameterValue ("origami_lfo2Shape");

        // ---- Voice ----
        pVoiceMode         = apvts.getRawParameterValue ("origami_voiceMode");
        pGlide             = apvts.getRawParameterValue ("origami_glide");

        // ---- Macros ----
        pMacroFold         = apvts.getRawParameterValue ("origami_macroFold");
        pMacroMotion       = apvts.getRawParameterValue ("origami_macroMotion");
        pMacroCoupling     = apvts.getRawParameterValue ("origami_macroCoupling");
        pMacroSpace        = apvts.getRawParameterValue ("origami_macroSpace");
    }

    //==========================================================================
    //  SynthEngine Interface -- Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Origami"; }

    juce::Colour getAccentColour() const override
    {
        // Vermillion Fold #E63946 -- a deep red-orange evoking the vermillion
        // ink of traditional Japanese washi paper and the sharp crease lines
        // of origami folds. In the water column, it's the warm flash of
        // sunlight catching a paper boat on the surface current.
        return juce::Colour (0xFFE63946);
    }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount; }

private:

    SilenceGate silenceGate;

    //==========================================================================
    //  Helper: Safe Parameter Load
    //==========================================================================

    static float loadParam (std::atomic<float>* paramPointer, float fallback) noexcept
    {
        return (paramPointer != nullptr) ? paramPointer->load() : fallback;
    }

    //==========================================================================
    //  FFT -- Radix-2 Decimation-In-Time, In-Place
    //
    //  Classic Cooley-Tukey algorithm (1965). The radix-2 DIT approach
    //  recursively splits the N-point DFT into two N/2-point DFTs using
    //  the even/odd index decomposition, achieving O(N log N) complexity
    //  versus the naive O(N^2) DFT.
    //
    //  Pre-computed bit-reversal tables and twiddle factors avoid redundant
    //  computation in the inner loop -- critical for real-time performance
    //  with 8 concurrent voice FFTs.
    //==========================================================================

    void buildBitReversalTable()
    {
        int fftLength = kFFTSize;

        // Compute log2(N) for bit-reversal depth
        int log2N = 0;
        {
            int temp = fftLength;
            while (temp > 1) { temp >>= 1; ++log2N; }
        }

        // Build the bit-reversal permutation table.
        // Bit-reversal reordering is required because the DIT algorithm
        // produces outputs in bit-reversed order of the input indices.
        for (int i = 0; i < fftLength; ++i)
        {
            int reversed = 0;
            int value = i;
            for (int bit = 0; bit < log2N; ++bit)
            {
                reversed = (reversed << 1) | (value & 1);
                value >>= 1;
            }
            bitReversalTable[static_cast<size_t> (i)] = reversed;
        }

        // Pre-compute twiddle factors (complex roots of unity).
        // W_N^k = e^(-j * 2*PI * k / N) = cos(angle) + j*sin(angle)
        // Only N/2 factors needed due to periodicity.
        for (int k = 0; k < kFFTSize / 2; ++k)
        {
            float angle = -kTwoPi * static_cast<float> (k) / static_cast<float> (kFFTSize);
            twiddleFactorsReal[static_cast<size_t> (k)] = std::cos (angle);
            twiddleFactorsImaginary[static_cast<size_t> (k)] = std::sin (angle);
        }
    }

    // Forward FFT: time domain -> frequency domain
    void fftForward (float* realPart, float* imaginaryPart) const noexcept
    {
        int fftLength = kFFTSize;

        // ---- Bit-reversal permutation ----
        for (int i = 0; i < fftLength; ++i)
        {
            int j = bitReversalTable[static_cast<size_t> (i)];
            if (j > i)
            {
                std::swap (realPart[i], realPart[j]);
                std::swap (imaginaryPart[i], imaginaryPart[j]);
            }
        }

        // ---- Butterfly stages ----
        // Each stage doubles the sub-DFT size: 2, 4, 8, ..., N
        for (int stageSize = 2; stageSize <= fftLength; stageSize *= 2)
        {
            int halfStage = stageSize / 2;
            int twiddleStep = fftLength / stageSize;    // Stride through twiddle table

            for (int groupStart = 0; groupStart < fftLength; groupStart += stageSize)
            {
                for (int butterfly = 0; butterfly < halfStage; ++butterfly)
                {
                    int twiddleIndex = butterfly * twiddleStep;
                    float twiddleReal = twiddleFactorsReal[static_cast<size_t> (twiddleIndex)];
                    float twiddleImag = twiddleFactorsImaginary[static_cast<size_t> (twiddleIndex)];

                    int evenIndex = groupStart + butterfly;
                    int oddIndex  = groupStart + butterfly + halfStage;

                    // Complex multiply: twiddle * odd element
                    float tempReal = realPart[oddIndex] * twiddleReal - imaginaryPart[oddIndex] * twiddleImag;
                    float tempImag = realPart[oddIndex] * twiddleImag + imaginaryPart[oddIndex] * twiddleReal;

                    // Butterfly: even +/- (twiddle * odd)
                    realPart[oddIndex]      = realPart[evenIndex] - tempReal;
                    imaginaryPart[oddIndex] = imaginaryPart[evenIndex] - tempImag;
                    realPart[evenIndex]      += tempReal;
                    imaginaryPart[evenIndex] += tempImag;
                }
            }
        }
    }

    // Inverse FFT: frequency domain -> time domain.
    // Uses the conjugate-forward-conjugate trick: IFFT(X) = conj(FFT(conj(X))) / N
    void fftInverse (float* realPart, float* imaginaryPart) const noexcept
    {
        int fftLength = kFFTSize;

        // Conjugate input (negate imaginary part)
        for (int i = 0; i < fftLength; ++i)
            imaginaryPart[i] = -imaginaryPart[i];

        // Forward FFT on conjugated input
        fftForward (realPart, imaginaryPart);

        // Conjugate and scale by 1/N
        float inverseN = 1.0f / static_cast<float> (fftLength);
        for (int i = 0; i < fftLength; ++i)
        {
            realPart[i] *= inverseN;
            imaginaryPart[i] = -imaginaryPart[i] * inverseN;
        }
    }

    //==========================================================================
    //  STFT Processing -- The Heart of Spectral Folding
    //
    //  This is where ORIGAMI's paper-folding metaphor becomes literal DSP.
    //  The 8-step pipeline transforms time-domain audio into the frequency
    //  domain, applies spectral origami operations, then reconstructs the
    //  time-domain signal via overlap-add synthesis.
    //
    //  The pipeline runs once per hop (every 512 samples at 44.1kHz, i.e.
    //  ~86 times per second), making it computationally tractable even with
    //  8 concurrent polyphonic voices.
    //==========================================================================

    void processSTFT (OrigamiVoice& voice, float foldPoint, float foldDepth,
                      int foldCount, int operation, float rotate, float stretch,
                      bool freezeActive) noexcept
    {
        // --- Step 1: Window the input ---
        // Extract kFFTSize samples ending at the current write position,
        // multiplied by the Hann window for spectral leakage suppression.
        for (int i = 0; i < kFFTSize; ++i)
        {
            int readIndex = (voice.inputWritePosition - kFFTSize + i + kFFTSize) % kFFTSize;
            voice.windowedInput[static_cast<size_t> (i)] =
                voice.inputRingBuffer[static_cast<size_t> (readIndex)] * hannWindow[static_cast<size_t> (i)];
        }

        // --- Step 2: Forward FFT ---
        // Copy windowed input to FFT buffers (imaginary part initialized to zero
        // because our input is real-valued)
        for (int i = 0; i < kFFTSize; ++i)
        {
            voice.fftReal[static_cast<size_t> (i)] = voice.windowedInput[static_cast<size_t> (i)];
            voice.fftImaginary[static_cast<size_t> (i)] = 0.0f;
        }

        fftForward (voice.fftReal.data(), voice.fftImaginary.data());

        // --- Step 3: Extract magnitude and phase ---
        if (!freezeActive)
        {
            // Save previous phase for phase vocoder's inter-frame delta computation
            voice.analysisFrame.previousPhase = voice.analysisFrame.phase;

            for (int bin = 0; bin < kFFTHalf; ++bin)
            {
                float real = voice.fftReal[static_cast<size_t> (bin)];
                float imaginary = voice.fftImaginary[static_cast<size_t> (bin)];

                float magnitude = std::sqrt (real * real + imaginary * imaginary);
                // Clamp magnitude to floor to prevent denormals in subsequent
                // spectral operations that multiply/divide by magnitude values
                magnitude = std::max (magnitude, kMagnitudeFloor);

                float binPhase = std::atan2 (imaginary, real);

                voice.analysisFrame.magnitude[static_cast<size_t> (bin)] = magnitude;
                voice.analysisFrame.phase[static_cast<size_t> (bin)] = binPhase;

                // Phase vocoder: compute instantaneous frequency.
                // The expected phase advance per hop for bin k is:
                //   2*PI * k * hopSize / fftSize
                // Any deviation from this expected advance indicates the true
                // frequency differs from the bin center frequency.
                float phaseDelta = binPhase - voice.analysisFrame.previousPhase[static_cast<size_t> (bin)];
                float expectedPhaseAdvance = kTwoPi * static_cast<float> (bin) * static_cast<float> (kHopSize)
                                             / static_cast<float> (kFFTSize);
                float phaseDeviation = phaseDelta - expectedPhaseAdvance;

                // Wrap deviation to [-PI, PI] for correct frequency estimation
                phaseDeviation = phaseDeviation - kTwoPi * std::round (phaseDeviation / kTwoPi);

                // Instantaneous frequency = bin center frequency + deviation-derived offset
                voice.analysisFrame.instantaneousFreq[static_cast<size_t> (bin)] =
                    static_cast<float> (bin) * frequencyPerBin
                    + phaseDeviation * sampleRateFloat / (kTwoPi * static_cast<float> (kHopSize));
            }

            // Snapshot the current frame for potential freeze activation
            voice.frozenFrame = voice.analysisFrame;
            voice.hasFrozenFrame = true;
        }

        // Select source frame: frozen snapshot if freeze is active, else live analysis
        const SpectralFrame& sourceFrame = (freezeActive && voice.hasFrozenFrame)
                                            ? voice.frozenFrame
                                            : voice.analysisFrame;

        // --- Step 4: Apply spectral fold operations ---
        // Start with a copy of the source frame's magnitude and phase
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            voice.foldedMagnitude[static_cast<size_t> (bin)] = sourceFrame.magnitude[static_cast<size_t> (bin)];
            voice.foldedPhase[static_cast<size_t> (bin)] = sourceFrame.phase[static_cast<size_t> (bin)];
        }

        // Apply the selected spectral operation, cascaded foldCount times.
        // Each cascade shifts the fold point by +10% for richer, more complex
        // results -- like folding paper multiple times at slightly offset creases.
        for (int cascade = 0; cascade < foldCount; ++cascade)
        {
            float cascadedFoldPoint = clamp (foldPoint + static_cast<float> (cascade) * 0.1f, 0.0f, 0.99f);

            switch (operation)
            {
                case 0:  // FOLD -- reflect spectrum at fold point
                    applySpectralFold (voice.foldedMagnitude.data(), voice.foldedPhase.data(),
                                       cascadedFoldPoint, foldDepth);
                    break;
                case 1:  // MIRROR -- bilateral symmetry around fold point
                    applySpectralMirror (voice.foldedMagnitude.data(), voice.foldedPhase.data(),
                                         cascadedFoldPoint, foldDepth);
                    break;
                case 2:  // ROTATE -- circular shift of magnitude spectrum
                    applySpectralRotate (voice.foldedMagnitude.data(), voice.foldedPhase.data(),
                                         rotate, foldDepth);
                    break;
                case 3:  // STRETCH -- nonlinear frequency-axis warping
                    applySpectralStretch (voice.foldedMagnitude.data(), voice.foldedPhase.data(),
                                          stretch, foldDepth);
                    break;
                default:
                    break;
            }
        }

        // --- Step 5: Post-fold magnitude smoothing ---
        // 3-point triangular kernel reduces harsh spectral discontinuities
        // that the fold operations can introduce at bin boundaries.
        applyTriangularSmoothing (voice.foldedMagnitude.data());

        // --- Step 6: Reconstruct complex spectrum from modified magnitude + phase ---
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            float magnitude = std::max (voice.foldedMagnitude[static_cast<size_t> (bin)], kMagnitudeFloor);
            float binPhase  = voice.foldedPhase[static_cast<size_t> (bin)];

            voice.ifftReal[static_cast<size_t> (bin)] = magnitude * std::cos (binPhase);
            voice.ifftImaginary[static_cast<size_t> (bin)] = magnitude * std::sin (binPhase);
        }

        // Mirror conjugate for negative frequencies.
        // Real-valued signals have conjugate-symmetric spectra:
        // X[N-k] = conj(X[k]), so we only need to compute the positive half.
        for (int bin = 1; bin < kFFTSize / 2; ++bin)
        {
            int mirrorBin = kFFTSize - bin;
            voice.ifftReal[static_cast<size_t> (mirrorBin)] =  voice.ifftReal[static_cast<size_t> (bin)];
            voice.ifftImaginary[static_cast<size_t> (mirrorBin)] = -voice.ifftImaginary[static_cast<size_t> (bin)];
        }

        // --- Step 7: Inverse FFT ---
        fftInverse (voice.ifftReal.data(), voice.ifftImaginary.data());

        // --- Step 8: Window and overlap-add ---
        // Normalization factor for 4x overlap with Hann window.
        // The sum of squared Hann windows across 4 overlapping frames = 1.5,
        // so we scale by 2/3 to achieve unity gain in the overlap-add
        // reconstruction (the COLA condition for Hann with 75% overlap).
        constexpr float overlapAddNormalization = 2.0f / 3.0f;

        for (int i = 0; i < kFFTSize; ++i)
        {
            float resynthSample = voice.ifftReal[static_cast<size_t> (i)]
                                * hannWindow[static_cast<size_t> (i)]
                                * overlapAddNormalization;

            // Add to overlap accumulator at the correct circular position.
            // flushDenormal prevents accumulated near-zero values from causing
            // CPU spikes in the ongoing overlap-add summation.
            int outputPosition = (voice.inputWritePosition + i) % kFFTSize;
            voice.overlapAddAccumulator[static_cast<size_t> (outputPosition)] += flushDenormal (resynthSample);
        }
    }

    //==========================================================================
    //  Spectral Operations -- The Four Folds
    //
    //  Each operation transforms the magnitude spectrum in a way that has
    //  no time-domain equivalent, producing timbres unique to spectral
    //  processing. The fold metaphor is deliberate: like origami, these
    //  operations crease, mirror, rotate, and stretch the spectral "paper."
    //==========================================================================

    // FOLD: Reflect spectrum at the fold point.
    // Bins above the fold point are reflected back below it, constructively
    // adding to existing spectral content. This creates dense, metallic
    // timbres reminiscent of bell or gong spectra where partials pile up
    // in the lower frequency range.
    void applySpectralFold (float* magnitude, float* phase, float foldPoint, float depth) const noexcept
    {
        int foldBin = std::max (1, static_cast<int> (foldPoint * static_cast<float> (kFFTHalf - 1)));

        // Work with temporary arrays to avoid in-place corruption
        std::array<float, kFFTHalf> tempMagnitude {};
        std::array<float, kFFTHalf> tempPhase {};

        // Preserve original content below fold point
        for (int bin = 0; bin < foldBin && bin < kFFTHalf; ++bin)
        {
            tempMagnitude[static_cast<size_t> (bin)] = magnitude[bin];
            tempPhase[static_cast<size_t> (bin)]     = phase[bin];
        }

        // Reflect bins above fold point back below it (the "fold" crease)
        for (int bin = foldBin; bin < kFFTHalf; ++bin)
        {
            int distanceFromFold = bin - foldBin;
            int reflectedBin = foldBin - 1 - distanceFromFold;

            if (reflectedBin >= 0 && reflectedBin < kFFTHalf)
            {
                // Constructive magnitude addition (spectral energy piles up)
                tempMagnitude[static_cast<size_t> (reflectedBin)] += magnitude[bin] * depth;

                // Phase blending: 70/30 weighted average preserves the original
                // phase character while incorporating reflected content. Pure
                // averaging (50/50) produces more cancellation artifacts.
                tempPhase[static_cast<size_t> (reflectedBin)] =
                    tempPhase[static_cast<size_t> (reflectedBin)] * 0.7f + phase[bin] * 0.3f;
            }
        }

        // Attenuate content above fold point (energy has been folded below)
        for (int bin = foldBin; bin < kFFTHalf; ++bin)
        {
            tempMagnitude[static_cast<size_t> (bin)] = magnitude[bin] * (1.0f - depth);
            tempPhase[static_cast<size_t> (bin)]     = phase[bin];
        }

        // Write back with depth-controlled wet/dry blend
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            magnitude[bin] = lerp (magnitude[bin], tempMagnitude[static_cast<size_t> (bin)], depth);
            phase[bin]     = tempPhase[static_cast<size_t> (bin)];
        }
    }

    // MIRROR: Create bilateral symmetry around the fold point.
    // The spectrum below the fold point is reflected above it, creating
    // palindromic spectral shapes. Phase inversion on the mirrored content
    // produces interesting interference patterns and hollow, resonant tones.
    void applySpectralMirror (float* magnitude, float* phase, float foldPoint, float depth) const noexcept
    {
        int foldBin = std::max (1, static_cast<int> (foldPoint * static_cast<float> (kFFTHalf - 1)));

        std::array<float, kFFTHalf> tempMagnitude {};
        std::array<float, kFFTHalf> tempPhase {};

        // Below fold point: keep original spectrum intact
        for (int bin = 0; bin < foldBin && bin < kFFTHalf; ++bin)
        {
            tempMagnitude[static_cast<size_t> (bin)] = magnitude[bin];
            tempPhase[static_cast<size_t> (bin)]     = phase[bin];
        }

        // Above fold point: mirror from below (the "butterfly" fold)
        for (int bin = foldBin; bin < kFFTHalf; ++bin)
        {
            int distanceFromFold = bin - foldBin;
            int mirrorSourceBin = foldBin - 1 - distanceFromFold;

            if (mirrorSourceBin >= 0 && mirrorSourceBin < kFFTHalf)
            {
                tempMagnitude[static_cast<size_t> (bin)] = magnitude[static_cast<size_t> (mirrorSourceBin)];
                // Phase inversion creates destructive interference at the fold
                // point, producing characteristic notches and hollow timbres
                tempPhase[static_cast<size_t> (bin)] = -phase[static_cast<size_t> (mirrorSourceBin)];
            }
            else
            {
                tempMagnitude[static_cast<size_t> (bin)] = kMagnitudeFloor;
                tempPhase[static_cast<size_t> (bin)]     = 0.0f;
            }
        }

        // Blend with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            magnitude[bin] = lerp (magnitude[bin], tempMagnitude[static_cast<size_t> (bin)], depth);
            phase[bin]     = lerp (phase[bin],     tempPhase[static_cast<size_t> (bin)],     depth);
        }
    }

    // ROTATE: Circular shift of magnitude spectrum.
    // Shifts all frequency bins by a proportional amount, wrapping around
    // at the Nyquist boundary. This produces pitch-shifting-like effects
    // with inharmonic overtones -- the harmonic series is translated rather
    // than scaled, destroying the integer-ratio relationship between partials.
    // Linear interpolation between adjacent bins prevents spectral staircasing.
    void applySpectralRotate (float* magnitude, float* phase, float rotateAmount, float depth) const noexcept
    {
        // Map [-1, 1] to bin shift range [-kFFTHalf/2, +kFFTHalf/2]
        // Full rotation (amount = 1.0) shifts by half the spectrum width
        float binShift = rotateAmount * static_cast<float> (kFFTHalf / 2);

        std::array<float, kFFTHalf> tempMagnitude {};
        std::array<float, kFFTHalf> tempPhase {};
        tempMagnitude.fill (kMagnitudeFloor);
        tempPhase.fill (0.0f);

        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            float destinationBin = static_cast<float> (bin) + binShift;

            // Circular wrap: bins that shift past Nyquist wrap to DC and vice versa
            while (destinationBin < 0.0f) destinationBin += static_cast<float> (kFFTHalf);
            while (destinationBin >= static_cast<float> (kFFTHalf)) destinationBin -= static_cast<float> (kFFTHalf);

            // Linear interpolation between adjacent destination bins
            int lowerBin = static_cast<int> (destinationBin);
            int upperBin = (lowerBin + 1) % kFFTHalf;
            float interpolationFraction = destinationBin - static_cast<float> (lowerBin);

            if (lowerBin >= 0 && lowerBin < kFFTHalf)
            {
                tempMagnitude[static_cast<size_t> (lowerBin)] += magnitude[bin] * (1.0f - interpolationFraction);
                tempPhase[static_cast<size_t> (lowerBin)]      = phase[bin];
            }
            if (upperBin >= 0 && upperBin < kFFTHalf)
            {
                tempMagnitude[static_cast<size_t> (upperBin)] += magnitude[bin] * interpolationFraction;
                // Only overwrite phase if this bin is now the dominant contributor
                if (tempMagnitude[static_cast<size_t> (upperBin)] < magnitude[bin] * interpolationFraction * 1.1f)
                    tempPhase[static_cast<size_t> (upperBin)] = phase[bin];
            }
        }

        // Blend with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            magnitude[bin] = lerp (magnitude[bin], std::max (tempMagnitude[static_cast<size_t> (bin)], kMagnitudeFloor), depth);
            phase[bin]     = lerp (phase[bin],     tempPhase[static_cast<size_t> (bin)], depth);
        }
    }

    // STRETCH: Nonlinear frequency-axis warping.
    // Applies a power-law warp to the frequency axis:
    //   positive stretch -> exponent > 1 -> compresses upper frequencies,
    //     expands lower frequencies -> darker timbre
    //   negative stretch -> exponent < 1 -> compresses lower frequencies,
    //     expands upper frequencies -> brighter timbre
    // This is analogous to nonlinear pitch scaling in physical modeling --
    // it changes the spacing between partials, making harmonic sounds
    // progressively more inharmonic.
    void applySpectralStretch (float* magnitude, float* phase, float stretchAmount, float depth) const noexcept
    {
        std::array<float, kFFTHalf> tempMagnitude {};
        std::array<float, kFFTHalf> tempPhase {};
        tempMagnitude.fill (kMagnitudeFloor);
        tempPhase.fill (0.0f);

        // Map stretch [-1, 1] to warping exponent [0.5, 2.0] via 2^stretch.
        // This exponential mapping ensures:
        //   stretch =  0 -> exponent = 1.0  (identity, no change)
        //   stretch = +1 -> exponent = 2.0  (square-law: darken)
        //   stretch = -1 -> exponent = 0.5  (square-root: brighten)
        float warpExponent = std::pow (2.0f, stretchAmount);

        for (int bin = 1; bin < kFFTHalf; ++bin)
        {
            // Normalized frequency position [0, 1]
            float normalizedFrequency = static_cast<float> (bin) / static_cast<float> (kFFTHalf - 1);

            // Apply power-law warp: f_out = f_in ^ exponent
            float warpedFrequency = std::pow (normalizedFrequency, warpExponent);

            // Map warped frequency back to bin index
            float warpedBinPosition = warpedFrequency * static_cast<float> (kFFTHalf - 1);

            // Linear interpolation to nearest bins
            int lowerBin = static_cast<int> (warpedBinPosition);
            int upperBin = lowerBin + 1;
            float interpolationFraction = warpedBinPosition - static_cast<float> (lowerBin);

            if (lowerBin >= 0 && lowerBin < kFFTHalf)
            {
                tempMagnitude[static_cast<size_t> (lowerBin)] += magnitude[bin] * (1.0f - interpolationFraction);
                tempPhase[static_cast<size_t> (lowerBin)]      = phase[bin];
            }
            if (upperBin >= 0 && upperBin < kFFTHalf)
            {
                tempMagnitude[static_cast<size_t> (upperBin)] += magnitude[bin] * interpolationFraction;
                tempPhase[static_cast<size_t> (upperBin)]      = phase[bin];
            }
        }

        // Preserve DC bin (bin 0) unchanged
        tempMagnitude[0] = magnitude[0];
        tempPhase[0]     = phase[0];

        // Blend with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            magnitude[bin] = lerp (magnitude[bin], std::max (tempMagnitude[static_cast<size_t> (bin)], kMagnitudeFloor), depth);
            phase[bin]     = lerp (phase[bin],     tempPhase[static_cast<size_t> (bin)], depth);
        }
    }

    //==========================================================================
    //  Triangular Magnitude Smoothing
    //
    //  3-point triangular (Bartlett) kernel: [0.25, 0.5, 0.25]
    //  Smooths harsh spectral discontinuities introduced by the fold
    //  operations, reducing audible artifacts at bin boundaries while
    //  preserving the overall spectral shape. Applied in-place with a
    //  single-pass sliding window.
    //==========================================================================

    static void applyTriangularSmoothing (float* magnitude) noexcept
    {
        // Triangular kernel weights: center = 0.5 (half weight),
        // neighbors = 0.25 each (quarter weight). Sum = 1.0 (unity gain).
        float previousBin = magnitude[0];
        for (int bin = 1; bin < kFFTHalf - 1; ++bin)
        {
            float currentBin = magnitude[bin];
            float smoothedValue = previousBin * 0.25f + currentBin * 0.5f + magnitude[bin + 1] * 0.25f;
            magnitude[bin] = std::max (smoothedValue, kMagnitudeFloor);
            previousBin = currentBin;
        }
    }

    //==========================================================================
    //  MIDI Note Handling
    //==========================================================================

    void handleNoteOn (int noteNumber, float velocity, int maxPolyphony,
                       bool monoMode, bool legatoMode, float glideCoefficient,
                       float ampAttack, float ampDecay, float ampSustain, float ampRelease,
                       float foldAttack, float foldDecay, float foldSustain, float foldRelease,
                       float lfo1Rate, float lfo1Depth, int lfo1Shape,
                       float lfo2Rate, float lfo2Depth, int lfo2Shape)
    {
        float frequency = midiNoteToFrequency (static_cast<float> (noteNumber));

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasAlreadyActive = voice.active;

            voice.targetFrequency = frequency;

            if (legatoMode && wasAlreadyActive)
            {
                // Legato: glide to new pitch without retriggering envelopes
                voice.glideCoefficient = glideCoefficient;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
            }
            else
            {
                // Full retrigger
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceAllocationCounter++;
                voice.currentFrequency = frequency;
                voice.glideCoefficient = glideCoefficient;
                voice.sawPhase = 0.0f;
                voice.squarePhase = 0.0f;
                voice.isFadingOut = false;
                voice.crossfadeGain = 1.0f;

                voice.ampEnvelope.setParams (ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
                voice.ampEnvelope.noteOn();
                voice.foldEnvelope.setParams (foldAttack, foldDecay, foldSustain, foldRelease, sampleRateFloat);
                voice.foldEnvelope.noteOn();

                voice.lfo1.setRate (lfo1Rate, sampleRateFloat);
                voice.lfo1.setShape (lfo1Shape);
                voice.lfo2.setRate (lfo2Rate, sampleRateFloat);
                voice.lfo2.setShape (lfo2Shape);
            }
            return;
        }

        // ---- Polyphonic mode ----
        int voiceIndex = findFreeVoice (maxPolyphony);
        auto& voice = voices[static_cast<size_t> (voiceIndex)];

        // If stealing an active voice, initiate crossfade out
        if (voice.active)
        {
            voice.isFadingOut = true;
            // Cap fade gain at 0.5 to speed up the steal transition
            voice.crossfadeGain = std::min (voice.crossfadeGain, 0.5f);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceAllocationCounter++;
        voice.currentFrequency = frequency;
        voice.targetFrequency = frequency;
        voice.glideCoefficient = 1.0f;      // No glide in poly mode (instant pitch)
        voice.sawPhase = 0.0f;
        voice.squarePhase = 0.0f;
        voice.isFadingOut = false;
        voice.crossfadeGain = 1.0f;
        voice.hopSampleCounter = 0;
        voice.hasFrozenFrame = false;

        // Clear STFT buffers for a clean spectral slate
        voice.inputRingBuffer.fill (0.0f);
        voice.overlapAddAccumulator.fill (0.0f);
        voice.analysisFrame.clear();

        voice.ampEnvelope.setParams (ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
        voice.ampEnvelope.noteOn();
        voice.foldEnvelope.setParams (foldAttack, foldDecay, foldSustain, foldRelease, sampleRateFloat);
        voice.foldEnvelope.noteOn();

        voice.lfo1.setRate (lfo1Rate, sampleRateFloat);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate (lfo2Rate, sampleRateFloat);
        voice.lfo2.setShape (lfo2Shape);
        voice.lfo2.reset();

        // Post-FFT smoothing filter: 18kHz lowpass at gentle Q to suppress
        // any residual spectral artifacts above the audible range
        voice.postFilter.reset();
        voice.postFilter.setMode (CytomicSVF::Mode::LowPass);
        voice.postFilter.setCoefficients (18000.0f, 0.1f, storedSampleRate);
    }

    void handleNoteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.isFadingOut)
            {
                voice.ampEnvelope.noteOff();
                voice.foldEnvelope.noteOff();
            }
        }
    }

    // Find the best voice to allocate: prefer inactive voices, then steal
    // the oldest active voice (Least Recently Used strategy).
    int findFreeVoice (int maxPolyphony) const
    {
        int polyphonyLimit = std::min (maxPolyphony, kMaxVoices);

        // First pass: find an inactive voice within the polyphony limit
        for (int i = 0; i < polyphonyLimit; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // Second pass: LRU voice stealing -- find the oldest active voice
        int oldestVoiceIndex = 0;
        uint64_t oldestTimestamp = UINT64_MAX;
        for (int i = 0; i < polyphonyLimit; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTimestamp)
            {
                oldestTimestamp = voices[static_cast<size_t> (i)].startTime;
                oldestVoiceIndex = i;
            }
        }
        return oldestVoiceIndex;
    }

    // Standard MIDI-to-frequency conversion using equal temperament.
    // A4 (MIDI note 69) = 440 Hz, 12 semitones per octave.
    static float midiNoteToFrequency (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    //  Member Data
    //==========================================================================

    // ---- Sample Rate ----
    double storedSampleRate = 44100.0;               // Full-precision sample rate for filter coefficient computation
    float sampleRateFloat = 44100.0f;                // Float-precision sample rate for per-sample DSP

    // ---- Control Smoothing ----
    float parameterSmoothingCoefficient = 0.1f;      // Per-sample smoothing rate (5ms time constant)
    float voiceCrossfadeRate = 0.01f;                 // Per-sample fade rate for voice stealing (5ms)
    float frequencyPerBin = 44100.0f / static_cast<float> (kFFTSize);   // Hz per FFT bin

    // ---- Voice Pool ----
    std::array<OrigamiVoice, kMaxVoices> voices;
    uint64_t voiceAllocationCounter = 0;             // Monotonic counter for LRU voice stealing
    std::atomic<int> activeVoiceCount { 0 };          // Current number of sounding voices (written audio thread, read UI thread)

    // ---- Pre-Computed Tables ----
    std::array<float, kFFTSize> hannWindow {};                      // Hann analysis/synthesis window
    std::array<int, kFFTSize> bitReversalTable {};                  // FFT bit-reversal permutation indices
    std::array<float, kFFTSize / 2> twiddleFactorsReal {};          // FFT twiddle factors (cosine)
    std::array<float, kFFTSize / 2> twiddleFactorsImaginary {};     // FFT twiddle factors (sine)

    // ---- Smoothed Control Parameters ----
    float smoothedFoldPoint = 0.5f;
    float smoothedFoldDepth = 0.5f;
    float smoothedRotate = 0.0f;
    float smoothedStretch = 0.0f;

    // D006: aftertouch — pressure increases fold depth (more spectral shimmer on pressure)
    PolyAftertouch aftertouch;

    // D006: mod wheel — CC1 increases STFT fold depth (more spectral processing with wheel; sensitivity 0.3)
    float modWheelValue = 0.0f;

    // ---- Coupling State ----
    float envelopeOutput = 0.0f;                      // Peak envelope level for amplitude coupling output
    float couplingFoldDepthModulation = 0.0f;          // Accumulated fold depth offset from external engines
    float couplingFoldPointModulation = 0.0f;          // Accumulated fold point offset from external engines
    float couplingFreezeTrigger = 0.0f;                // Binary freeze trigger from rhythm coupling
    float couplingSourceModulation = 0.0f;             // Source blend offset from audio coupling

    // ---- Output Cache (for inter-engine coupling reads) ----
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- Coupling Input Buffer (receives external audio) ----
    std::vector<float> couplingInputBuffer;

    // ---- Cached APVTS Parameter Pointers ----
    // These are set once during attachParameters() and read per-block
    // via loadParam(). nullptr until attached.

    // Core spectral parameters
    std::atomic<float>* pFoldPoint = nullptr;
    std::atomic<float>* pFoldDepth = nullptr;
    std::atomic<float>* pFoldCount = nullptr;
    std::atomic<float>* pOperation = nullptr;
    std::atomic<float>* pRotate = nullptr;
    std::atomic<float>* pStretch = nullptr;
    std::atomic<float>* pFreeze = nullptr;
    std::atomic<float>* pSource = nullptr;
    std::atomic<float>* pOscMix = nullptr;
    std::atomic<float>* pLevel = nullptr;

    // Amplitude envelope
    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    // Fold envelope
    std::atomic<float>* pFoldEnvAttack = nullptr;
    std::atomic<float>* pFoldEnvDecay = nullptr;
    std::atomic<float>* pFoldEnvSustain = nullptr;
    std::atomic<float>* pFoldEnvRelease = nullptr;

    // LFOs
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    // Voice parameters
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;

    // Macros
    std::atomic<float>* pMacroFold = nullptr;
    std::atomic<float>* pMacroMotion = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;
};

} // namespace xomnibus

