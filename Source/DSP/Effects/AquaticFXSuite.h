#pragma once
#include "MultibandCompressor.h"
#include "../FastMath.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <cmath>

namespace xomnibus {

//==============================================================================
// AquaticFXSuite — 6-stage aquatic-themed master effects chain.
//
// Signal chain (fixed order):
//   1. Fathom    — 3-band pressure compressor (MultibandCompressor + character morph)
//   2. Drift     — Brownian-walk multi-voice chorus (Ornstein-Uhlenbeck LFOs)
//   3. Tide      — Tremolo / auto-filter + tempo-sync LFO
//   4. Reef      — 4-channel Householder FDN early reflections
//   5. Surface   — Cytomic TPT SVF high-shelf sweep (COUPLING macro target)
//   6. Biolume   — Half-wave rectifier + HP + saturation shimmer layer
//
// All stages bypass at zero CPU when their mix = 0.
// Full spec: Docs/specs/aquatic_fx_suite_spec.md
//
// Integration: called from MasterFXChain::processBlock() between stage 19
// (Bus Compressor) and stage 20 (Brickwall Limiter).
//
// Parameter prefix: aqua_
// Macro targets: CHARACTER(M1) Fathom+Biolume, SPACE(M2) Drift+Reef,
//                MOVEMENT(M3) Drift+Tide, COUPLING(M4) Surface
//==============================================================================
class AquaticFXSuite
{
public:
    AquaticFXSuite() = default;

    //--------------------------------------------------------------------------
    // Lifecycle
    //--------------------------------------------------------------------------

    /// Prepare all 6 stages. Must be called before processBlock().
    /// @param sampleRate     Host sample rate in Hz. Never hardcode 44100.
    /// @param samplesPerBlock Maximum block size (for pre-allocation).
    void prepare (double sampleRate, int samplesPerBlock);

    /// Release all internal buffers.
    void reset();

    //--------------------------------------------------------------------------
    // Main processing entry point
    //--------------------------------------------------------------------------

    /// Process stereo audio through all 6 stages in order.
    /// Parameters are read from the cached pointers populated by cacheParameterPointers().
    /// @param L           Left channel write pointer.
    /// @param R           Right channel write pointer.
    /// @param numSamples  Number of samples to process.
    /// @param bpm         Host BPM (needed by Tide for tempo-sync). Pass 0 if unavailable.
    void processBlock (float* L, float* R, int numSamples, double bpm = 0.0);

    /// Cache all aqua_ parameter pointers once per prepareToPlay.
    /// Call this from MasterFXChain::cacheParameterPointers().
    void cacheParameterPointers (juce::AudioProcessorValueTreeState& apvts);

    //--------------------------------------------------------------------------
    // Static parameter layout factory
    // Call from XOmnibusProcessor::createParameterLayout() to add all aqua_ params.
    //--------------------------------------------------------------------------
    static void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout);

private:
    //==========================================================================
    // Inner DSP struct: Topology-Preserving Transform State Variable Filter
    // Shared by Stage 3 (Tide auto-filter) and Stage 5 (Surface shelf).
    // Reference: Andy Simper / Cytomic 2012, "Solving the Continuous SVF Equations
    //            Using Trapezoidal Integration and Magic Circle Algorithm"
    //==========================================================================
    struct TPT_SVF
    {
        // Integrator states
        float ic1 = 0.0f;
        float ic2 = 0.0f;

        // Precomputed coefficients (recomputed when fc or resonance changes)
        float a1 = 1.0f;
        float a2 = 0.0f;
        float a3 = 0.0f;
        float k  = 0.0f;    // 2 * (1 - resonance), range [0.02, 2.0]

        // Shelf-specific gain coefficient (for high/low shelf modes)
        float sqrtGain = 1.0f;  // sqrt(linear gain)
        float linGain  = 1.0f;  // linear gain

        /// Recompute coefficients. Call when fc or resonance changes (with smoothing).
        /// @param fc         Cutoff/pivot frequency in Hz.
        /// @param resonance  Resonance [0, 1] (maps to k = 2*(1-res)).
        /// @param sampleRate Host sample rate.
        void updateCoeffs (float fc, float resonance, double sampleRate);

        /// Update shelf gain coefficient (for HighShelf/LowShelf modes).
        void updateGain (float gainDb);

        /// Process one stereo sample pair.
        /// Returns high-shelf output (HP + sqrtGain*BP + linGain*LP).
        float processHighShelf (float input);

        /// Returns low-shelf output (linGain*(HP + sqrtGain*BP) + LP).
        float processLowShelf (float input);

        /// Returns bandpass output (for bell mode).
        float processBell (float input, float bellGainDb);

        /// Returns lowpass output (for Tide auto-filter).
        float processLP (float input);

        void reset() { ic1 = 0.0f; ic2 = 0.0f; }
    };

    //==========================================================================
    // Stage 1 — Fathom (Pressure Compression)
    //==========================================================================
    struct FathomStage
    {
        MultibandCompressor comp;
        float srSmooth = 0.0f;  // smoothed character value

        void prepare (double sampleRate) { comp.prepare (sampleRate); }
        void reset()                     { comp.reset(); srSmooth = 0.0f; }

        /// Process block. All parameter values are pre-read by the parent class.
        void processBlock (float* L, float* R, int numSamples,
                           float mix, float depth,
                           float lowGain, float midGain, float highGain,
                           float lowXover, float highXover,
                           float character);
    } fathom;

    //==========================================================================
    // Stage 2 — Drift (Brownian Chorus)
    //==========================================================================
    struct DriftStage
    {
        static constexpr int kMaxVoices = 6;
        static constexpr int kMaxDelayMs = 50;

        // Per-voice Brownian walk state
        struct Voice {
            std::vector<float> delayLine;
            int writePos   = 0;
            float walkPos  = 0.0f;   // current delay offset in ms
            float walkVel  = 0.0f;   // current velocity (Ornstein-Uhlenbeck)
            float phase    = 0.0f;   // stereo spread phase offset (radians)
            float hpState  = 0.0f;   // 1-pole HP filter state in feedback path
        };
        std::array<Voice, kMaxVoices> voices;

        double sr = 44100.0;

        void prepare (double sampleRate);
        void reset();
        void processBlock (float* L, float* R, int numSamples,
                           int numVoices, float depthMs, float rate,
                           float diffusion, float feedback,
                           float stereoSpread, float mix);
    } drift;

    //==========================================================================
    // Stage 3 — Tide (Tremolo / Auto-Filter)
    //==========================================================================
    struct TideStage
    {
        enum class Mode { Tremolo = 0, AutoFilter, Both };
        enum class Shape { Sine = 0, Triangle, SampleAndHold };

        // LFO state
        float lfoPhase   = 0.0f;
        float lfoPhaseR  = 0.0f;   // phase for R channel (offset by stereoPhase)
        float shState    = 0.0f;   // S&H current held value
        float shTimer    = 0.0f;   // S&H time accumulator (samples)
        float shSlew     = 0.0f;   // S&H slew state

        // Auto-filter SVF (stereo — one per channel)
        TPT_SVF svfL;
        TPT_SVF svfR;

        double sr = 44100.0;

        void prepare (double sampleRate);
        void reset();
        void processBlock (float* L, float* R, int numSamples,
                           float mix, Mode mode, float rate,
                           float depth, Shape shape,
                           bool syncEnabled, int syncDiv, double bpm,
                           float filterFreq, float filterRes,
                           float stereoPhase);

    private:
        float computeLFO (float& phase, Shape shape, float rateSamples);
        float computeSyncRate (int syncDiv, double bpm, double sampleRate);
    } tide;

    //==========================================================================
    // Stage 4 — Reef (Householder FDN Early Reflections)
    //==========================================================================
    struct ReefStage
    {
        static constexpr int kNumChannels = 4;

        struct DelayLine {
            std::vector<float> buffer;
            int writePos = 0;
            float dampState = 0.0f;  // 1-pole LP for HF damping
        };
        std::array<DelayLine, kNumChannels> delays;

        // Pre-delay buffers (stereo)
        std::vector<float> predelayL;
        std::vector<float> predelayR;
        int predelayWriteL = 0;
        int predelayWriteR = 0;

        // Allpass diffusers: 2 per channel, in series
        struct Allpass {
            std::vector<float> buffer;
            int writePos = 0;
            float g = 0.3f;
        };
        std::array<std::array<Allpass, 2>, kNumChannels> allpasses;

        double sr = 44100.0;

        // Prime-number base delay lengths at 44.1kHz (samples)
        static constexpr int kBaseLengths[kNumChannels] = { 1237, 1601, 1913, 2237 };

        void prepare (double sampleRate, int maxBlockSize);
        void reset();
        void processBlock (float* L, float* R, int numSamples,
                           float mix, float size, float decay,
                           float damping, float density,
                           float predelayMs, float width);

    private:
        /// Apply Householder mixing matrix (in-place, N=4).
        /// H[i][j] = delta(i,j) - 1/2  =>  out[i] = in[i] - 0.5 * sum(in)
        void householderMix (float* x);

        /// Read from a delay line with linear interpolation.
        float readDelayLinear (const DelayLine& dl, float delaySamples);
    } reef;

    //==========================================================================
    // Stage 5 — Surface (Cytomic SVF High-Shelf Sweep)
    //==========================================================================
    struct SurfaceStage
    {
        enum class Mode { HighShelf = 0, LowShelf, Bell };

        // Stereo SVF pair
        TPT_SVF svfL;
        TPT_SVF svfR;

        // Parameter smoothers (1-pole, 20ms time constant)
        float freqSmoothed  = 4000.0f;
        float gainSmoothed  = 0.0f;
        float smoothCoeff   = 0.0f;  // computed once in prepare()

        double sr = 44100.0;

        void prepare (double sampleRate);
        void reset();
        void processBlock (float* L, float* R, int numSamples,
                           float mix, float baseFreq, float gainDb,
                           float resonance, Mode mode,
                           float sweep, float sweepOctaves,
                           float coupling);
    } surface;

    //==========================================================================
    // Stage 6 — Biolume (Shimmer Saturation)
    //==========================================================================
    struct BiolumeStage
    {
        enum class SatMode { Soft = 0, Hard, Fold };

        // Level detector envelope for gate
        float envL = 0.0f;
        float envR = 0.0f;
        float attackCoeff  = 0.0f;
        float releaseCoeff = 0.0f;

        // HP filter states (L and R, 2-pole cascade)
        float hpL1 = 0.0f, hpL2 = 0.0f;
        float hpR1 = 0.0f, hpR2 = 0.0f;

        // Allpass decorrelation state (applied to R shimmer channel)
        float apState = 0.0f;

        double sr = 44100.0;

        void prepare (double sampleRate);
        void reset();
        void processBlock (float* L, float* R, int numSamples,
                           float mix, float thresholdDb, float drive,
                           float hpFreq, float shimmerBlend,
                           SatMode satMode, float spread,
                           float character);

    private:
        /// Apply selected saturation curve to sample.
        float applySaturation (float x, SatMode mode) const;

        /// Foldback saturation: reflects values outside [-ceiling, +ceiling].
        float foldback (float x, float ceiling) const;
    } biolume;

    //==========================================================================
    // Cached parameter pointers — resolved once in cacheParameterPointers()
    // All are nullable; null = parameter not yet registered (safe to skip).
    //==========================================================================
    struct CachedParams
    {
        // Stage 1: Fathom
        std::atomic<float>* fathomMix      = nullptr;
        std::atomic<float>* fathomDepth    = nullptr;
        std::atomic<float>* fathomLowGain  = nullptr;
        std::atomic<float>* fathomMidGain  = nullptr;
        std::atomic<float>* fathomHighGain = nullptr;
        std::atomic<float>* fathomLowXover = nullptr;
        std::atomic<float>* fathomHiXover  = nullptr;
        std::atomic<float>* fathomChar     = nullptr;  // M1 CHARACTER

        // Stage 2: Drift
        std::atomic<float>* driftMix       = nullptr;
        std::atomic<float>* driftVoices    = nullptr;
        std::atomic<float>* driftDepth     = nullptr;
        std::atomic<float>* driftRate      = nullptr;
        std::atomic<float>* driftDiffusion = nullptr;
        std::atomic<float>* driftFeedback  = nullptr;
        std::atomic<float>* driftSpread    = nullptr;
        std::atomic<float>* driftSpace     = nullptr;  // M2 SPACE
        std::atomic<float>* driftMovement  = nullptr;  // M3 MOVEMENT

        // Stage 3: Tide
        std::atomic<float>* tideMix        = nullptr;
        std::atomic<float>* tideMode       = nullptr;
        std::atomic<float>* tideRate       = nullptr;
        std::atomic<float>* tideDepth      = nullptr;
        std::atomic<float>* tideShape      = nullptr;
        std::atomic<float>* tideSyncEn     = nullptr;
        std::atomic<float>* tideSyncDiv    = nullptr;
        std::atomic<float>* tideFilterFreq = nullptr;
        std::atomic<float>* tideFilterRes  = nullptr;
        std::atomic<float>* tideStereoPhase= nullptr;
        std::atomic<float>* tideMovement   = nullptr;  // M3 MOVEMENT

        // Stage 4: Reef
        std::atomic<float>* reefMix        = nullptr;
        std::atomic<float>* reefSize       = nullptr;
        std::atomic<float>* reefDecay      = nullptr;
        std::atomic<float>* reefDamping    = nullptr;
        std::atomic<float>* reefDensity    = nullptr;
        std::atomic<float>* reefPredelay   = nullptr;
        std::atomic<float>* reefWidth      = nullptr;
        std::atomic<float>* reefSpace      = nullptr;  // M2 SPACE

        // Stage 5: Surface
        std::atomic<float>* surfaceMix     = nullptr;
        std::atomic<float>* surfaceFreq    = nullptr;
        std::atomic<float>* surfaceGain    = nullptr;
        std::atomic<float>* surfaceRes     = nullptr;
        std::atomic<float>* surfaceMode    = nullptr;
        std::atomic<float>* surfaceSweep   = nullptr;
        std::atomic<float>* surfaceSweepRng= nullptr;
        std::atomic<float>* surfaceCoupling= nullptr;  // M4 COUPLING

        // Stage 6: Biolume
        std::atomic<float>* biolumeMix     = nullptr;
        std::atomic<float>* biolumeThresh  = nullptr;
        std::atomic<float>* biolumeDrive   = nullptr;
        std::atomic<float>* biolumeHpFreq  = nullptr;
        std::atomic<float>* biolumeShimmer = nullptr;
        std::atomic<float>* biolumeSatMode = nullptr;
        std::atomic<float>* biolumeSpread  = nullptr;
        std::atomic<float>* biolumeChar    = nullptr;  // M1 CHARACTER
    } params;

    double sr          = 44100.0;
    int    blockSize   = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AquaticFXSuite)
};

} // namespace xomnibus
