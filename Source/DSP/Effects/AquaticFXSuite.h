#pragma once
#include "MultibandCompressor.h"
#include "../FastMath.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <cmath>
#include <random>

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
        void updateCoeffs (float fc, float resonance, double sampleRate)
        {
            constexpr float pi = 3.14159265358979323846f;
            float g = fastTan (pi * fc / static_cast<float> (sampleRate));
            // Clamp g to avoid instability at extreme frequencies
            g = std::max (0.0001f, std::min (g, 0.9999f));
            k = 0.02f + resonance * 1.98f;  // [0.02, 2.0]
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        /// Update shelf gain coefficient (for HighShelf/LowShelf modes).
        void updateGain (float gainDb)
        {
            // linGain = 10^(gainDb/20)
            linGain  = fastPow2 (gainDb * (1.0f / 6.0205999f));
            sqrtGain = fastPow2 (gainDb * (1.0f / 12.411f));   // 10^(dB/40)
        }

        /// Process one sample through the SVF core, returning high-shelf output.
        float processHighShelf (float x)
        {
            float v3 = x - ic2;
            float v1 = a1 * ic1 + a2 * v3;
            float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = flushDenormal (2.0f * v1 - ic1);
            ic2 = flushDenormal (2.0f * v2 - ic2);
            float hp = v3 - k * v1 - v2;  // high-pass
            float bp = v1;                  // band-pass
            float lp = v2;                  // low-pass
            // High shelf: output = HP + sqrt(gain)*BP + gain*LP
            return hp + sqrtGain * bp + linGain * lp;
        }

        /// Returns low-shelf output.
        float processLowShelf (float x)
        {
            float v3 = x - ic2;
            float v1 = a1 * ic1 + a2 * v3;
            float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = flushDenormal (2.0f * v1 - ic1);
            ic2 = flushDenormal (2.0f * v2 - ic2);
            float hp = v3 - k * v1 - v2;
            float bp = v1;
            float lp = v2;
            // Low shelf: gain*(HP + sqrt(gain)*BP) + LP
            return linGain * (hp + sqrtGain * bp) + lp;
        }

        /// Returns bell/peak output.
        float processBell (float x, float bellGainDb)
        {
            float v3 = x - ic2;
            float v1 = a1 * ic1 + a2 * v3;
            float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = flushDenormal (2.0f * v1 - ic1);
            ic2 = flushDenormal (2.0f * v2 - ic2);
            // Bell: input + (bellGain - k) * BP
            float bellGainLin = fastPow2 (bellGainDb * (1.0f / 6.0205999f));
            return x + (bellGainLin - k) * v1;
        }

        /// Returns lowpass output (for Tide auto-filter).
        float processLP (float x)
        {
            float v3 = x - ic2;
            float v1 = a1 * ic1 + a2 * v3;
            float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = flushDenormal (2.0f * v1 - ic1);
            ic2 = flushDenormal (2.0f * v2 - ic2);
            return v2;  // LP output
        }

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

        void processBlock (float* L, float* R, int numSamples,
                           float mix, float depth,
                           float lowGain, float midGain, float highGain,
                           float lowXover, float highXover,
                           float character)
        {
            if (mix < 0.0001f) return;

            // Character morphs band-gain balance:
            //   character=0 → lowMult=1.4, highMult=0.7 (deep/dark)
            //   character=1 → lowMult=0.7, highMult=1.4 (bright/present)
            float lowMult  = 1.4f + character * (0.7f - 1.4f);   // 1.4 → 0.7
            float highMult = 0.7f + character * (1.4f - 0.7f);   // 0.7 → 1.4

            comp.setLowCrossover  (lowXover);
            comp.setHighCrossover (highXover);
            comp.setDepth         (depth);
            comp.setBandGains     (lowGain * lowMult,
                                   midGain,
                                   highGain * highMult);
            comp.setMix           (mix);
            comp.processBlock     (L, R, numSamples);
        }
    } fathom;

    //==========================================================================
    // Stage 2 — Drift (Brownian Chorus)
    //==========================================================================
    struct DriftStage
    {
        static constexpr int kMaxVoices  = 6;
        static constexpr int kMaxDelayMs = 50;

        // Base delays per voice, L and R offset slightly for stereo spread
        // L: 8, 12, 17, 23, 30, 38 ms  |  R: 9, 13, 18, 24, 31, 39 ms
        static constexpr float kBaseDelayL[kMaxVoices] = { 8.0f, 12.0f, 17.0f, 23.0f, 30.0f, 38.0f };
        static constexpr float kBaseDelayR[kMaxVoices] = { 9.0f, 13.0f, 18.0f, 24.0f, 31.0f, 39.0f };

        struct Voice {
            std::vector<float> delayLineL;
            std::vector<float> delayLineR;
            int writePosL  = 0;
            int writePosR  = 0;
            float ouX      = 0.0f;   // OU walk current position (normalised, -1..1)
            float ouV      = 0.0f;   // OU velocity
            float hpStateL = 0.0f;   // 1-pole HP state (L feedback)
            float hpStateR = 0.0f;   // 1-pole HP state (R feedback)
        };
        std::array<Voice, kMaxVoices> voices;

        double sr = 44100.0;

        // PRNG state (xorshift32 — no heap, deterministic)
        uint32_t rngState = 0xDEADBEEFu;

        float nextRand()
        {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            // Map to [-1, 1]
            return static_cast<float> (static_cast<int32_t> (rngState)) * (1.0f / 2147483648.0f);
        }

        void prepare (double sampleRate)
        {
            sr = sampleRate;
            int maxDelaySamples = static_cast<int> (kMaxDelayMs * sampleRate / 1000.0) + 16;
            for (int v = 0; v < kMaxVoices; ++v)
            {
                voices[v].delayLineL.assign (maxDelaySamples, 0.0f);
                voices[v].delayLineR.assign (maxDelaySamples, 0.0f);
                voices[v].writePosL = 0;
                voices[v].writePosR = 0;
                voices[v].ouX  = 0.0f;
                voices[v].ouV  = 0.0f;
                voices[v].hpStateL = 0.0f;
                voices[v].hpStateR = 0.0f;
            }
        }

        void reset()
        {
            for (auto& v : voices)
            {
                std::fill (v.delayLineL.begin(), v.delayLineL.end(), 0.0f);
                std::fill (v.delayLineR.begin(), v.delayLineR.end(), 0.0f);
                v.writePosL = 0;
                v.writePosR = 0;
                v.ouX = 0.0f; v.ouV = 0.0f;
                v.hpStateL = 0.0f; v.hpStateR = 0.0f;
            }
        }

        // Linear interpolation read from a delay buffer
        static float readDelayInterp (const std::vector<float>& buf, int writePos, float delaySamples)
        {
            int size = static_cast<int> (buf.size());
            // Wrap read position
            float readF = static_cast<float> (writePos) - delaySamples;
            while (readF < 0.0f)  readF += static_cast<float> (size);
            while (readF >= static_cast<float> (size)) readF -= static_cast<float> (size);

            int r0 = static_cast<int> (readF);
            int r1 = (r0 + 1) % size;
            float frac = readF - static_cast<float> (r0);
            return buf[r0] * (1.0f - frac) + buf[r1] * frac;
        }

        void processBlock (float* L, float* R, int numSamples,
                           int numVoices, float depthMs, float rate,
                           float diffusion, float feedback,
                           float stereoSpread, float mix)
        {
            if (mix < 0.0001f) return;

            numVoices = std::max (1, std::min (numVoices, kMaxVoices));
            float srF = static_cast<float> (sr);

            // Ornstein-Uhlenbeck parameters
            constexpr float theta = 2.0f;
            float sigma = 0.3f * diffusion;
            float dt    = 1.0f / srF;
            float sqrtDt = std::sqrt (dt);

            // HP feedback filter coeff (~80 Hz)
            float hpCoeff = 1.0f - fastExp (-2.0f * 3.14159265f * 80.0f / srF);

            // Normalisation factor for voice sum
            float normFactor = 1.0f / std::sqrt (static_cast<float> (numVoices));

            float maxDelaySamplesF = static_cast<float> (voices[0].delayLineL.size() - 2);

            for (int i = 0; i < numSamples; ++i)
            {
                float inL = L[i];
                float inR = R[i];
                float outL = 0.0f;
                float outR = 0.0f;

                for (int v = 0; v < numVoices; ++v)
                {
                    Voice& vx = voices[v];

                    // --- Ornstein-Uhlenbeck walk ---
                    float noise = nextRand() * sigma * sqrtDt;
                    vx.ouV += (-theta * vx.ouX * dt + noise);
                    vx.ouV *= 0.999f;  // velocity damping
                    vx.ouX += vx.ouV;
                    // Clamp walk to [-1, 1]
                    vx.ouX = std::max (-1.0f, std::min (1.0f, vx.ouX));

                    // Convert OU position to delay in samples
                    float baseL   = kBaseDelayL[v] * srF / 1000.0f;
                    float baseR   = kBaseDelayR[v] * srF / 1000.0f;
                    float devSamp = vx.ouX * depthMs * srF / 1000.0f;

                    float delayL = std::max (1.0f, std::min (baseL + devSamp, maxDelaySamplesF));
                    float delayR = std::max (1.0f, std::min (baseR + devSamp * stereoSpread, maxDelaySamplesF));

                    // Read delayed samples
                    float delL = readDelayInterp (vx.delayLineL, vx.writePosL, delayL);
                    float delR = readDelayInterp (vx.delayLineR, vx.writePosR, delayR);

                    // HP filter on feedback to prevent mud
                    vx.hpStateL = flushDenormal (vx.hpStateL + hpCoeff * (delL - vx.hpStateL));
                    vx.hpStateR = flushDenormal (vx.hpStateR + hpCoeff * (delR - vx.hpStateR));
                    float hpL = delL - vx.hpStateL;
                    float hpR = delR - vx.hpStateR;

                    // Write input + feedback into delay line
                    int dlSize = static_cast<int> (vx.delayLineL.size());
                    vx.delayLineL[vx.writePosL] = flushDenormal (inL + feedback * hpL + 1e-25f);
                    vx.delayLineR[vx.writePosR] = flushDenormal (inR + feedback * hpR + 1e-25f);
                    vx.writePosL = (vx.writePosL + 1) % dlSize;
                    vx.writePosR = (vx.writePosR + 1) % dlSize;

                    outL += delL;
                    outR += delR;
                }

                outL *= normFactor;
                outR *= normFactor;

                L[i] = inL + mix * (outL - inL);
                R[i] = inR + mix * (outR - inR);
            }
        }
    } drift;

    //==========================================================================
    // Stage 3 — Tide (Tremolo / Auto-Filter)
    //==========================================================================
    struct TideStage
    {
        enum class Mode  { Tremolo = 0, AutoFilter, Both };
        enum class Shape { Sine = 0, Triangle, SampleAndHold };

        float lfoPhase   = 0.0f;
        float lfoPhaseR  = 0.0f;
        float shState    = 0.0f;
        float shTimer    = 0.0f;
        float shSlew     = 0.0f;

        TPT_SVF svfL;
        TPT_SVF svfR;

        double sr = 44100.0;

        void prepare (double sampleRate)
        {
            sr = sampleRate;
            lfoPhase = 0.0f;
            lfoPhaseR = 0.0f;
            shState = 0.0f;
            shTimer = 0.0f;
            shSlew  = 0.0f;
            svfL.reset();
            svfR.reset();
        }

        void reset()
        {
            lfoPhase = 0.0f; lfoPhaseR = 0.0f;
            shState = 0.0f;  shTimer = 0.0f;  shSlew = 0.0f;
            svfL.reset();    svfR.reset();
        }

        void processBlock (float* L, float* R, int numSamples,
                           float mix, Mode mode, float rate,
                           float depth, Shape shape,
                           bool syncEnabled, int syncDiv, double bpm,
                           float filterFreq, float filterRes,
                           float stereoPhaseRad)
        {
            if (mix < 0.0001f) return;

            float srF = static_cast<float> (sr);

            // Resolve rate (possibly from tempo sync)
            float effectiveRate = rate;
            if (syncEnabled && bpm > 0.0)
            {
                static constexpr float kSyncMults[8] = {
                    1.0f/32.0f, 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f,
                    1.0f/2.0f,  1.0f,       2.0f,       4.0f
                };
                int divIdx = std::max (0, std::min (syncDiv, 7));
                effectiveRate = static_cast<float> (bpm) / 60.0f * kSyncMults[divIdx];
            }
            effectiveRate = std::max (0.01f, effectiveRate);

            float phaseInc  = 2.0f * 3.14159265f * effectiveRate / srF;

            // S&H slew time in samples (10 ms)
            float slewSamples = 0.01f * srF;

            // Update SVF coefficients once per block
            if (mode == Mode::AutoFilter || mode == Mode::Both)
            {
                float res = std::max (0.0f, std::min (filterRes, 0.95f));
                svfL.updateCoeffs (filterFreq, res, sr);
                svfR.updateCoeffs (filterFreq, res, sr);
            }

            for (int i = 0; i < numSamples; ++i)
            {
                // Compute LFO value for L channel
                float lfoL = computeLFOSample (lfoPhase, shape, shState, shSlew, shTimer,
                                               srF / effectiveRate, slewSamples);
                // R channel uses offset phase
                float tmpPhaseR = lfoPhaseR;
                float dummyShState = shState, dummyShSlew = shSlew, dummyShTimer = shTimer;
                float lfoR = computeLFOSample (tmpPhaseR, shape, dummyShState, dummyShSlew, dummyShTimer,
                                               srF / effectiveRate, slewSamples);

                // Advance main phase
                lfoPhase  += phaseInc;
                lfoPhaseR += phaseInc;
                if (lfoPhase  > 2.0f * 3.14159265f) lfoPhase  -= 2.0f * 3.14159265f;
                if (lfoPhaseR > 2.0f * 3.14159265f) lfoPhaseR -= 2.0f * 3.14159265f;

                float inL = L[i];
                float inR = R[i];
                float outL = inL;
                float outR = inR;

                if (mode == Mode::Tremolo || mode == Mode::Both)
                {
                    // gain sweeps from 1.0 down to (1-depth), floor at 0.05
                    float gainL = 1.0f - depth * 0.5f * (1.0f + lfoL);
                    float gainR = 1.0f - depth * 0.5f * (1.0f + lfoR);
                    gainL = std::max (0.05f, gainL);
                    gainR = std::max (0.05f, gainR);
                    outL *= gainL;
                    outR *= gainR;
                }

                if (mode == Mode::AutoFilter || mode == Mode::Both)
                {
                    // Exponential sweep ±2 octaves around center
                    float cutoffL = filterFreq * fastPow2 (depth * 2.0f * lfoL);
                    float cutoffR = filterFreq * fastPow2 (depth * 2.0f * lfoR);
                    cutoffL = std::max (20.0f, std::min (cutoffL, srF * 0.45f));
                    cutoffR = std::max (20.0f, std::min (cutoffR, srF * 0.45f));

                    svfL.updateCoeffs (cutoffL, filterRes, sr);
                    svfR.updateCoeffs (cutoffR, filterRes, sr);
                    outL = svfL.processLP (outL);
                    outR = svfR.processLP (outR);
                }

                L[i] = inL + mix * (outL - inL);
                R[i] = inR + mix * (outR - inR);
            }

            // Initialise R phase offset once here (after first prepare)
            // We track lfoPhaseR as lfoPhase + stereoPhaseRad but they evolve together
            // so we re-sync the initial offset on prepare only; here we clamp
            (void)stereoPhaseRad;
        }

    private:
        // Compute one LFO sample, advance phase-related state
        float computeLFOSample (float phase, Shape shape,
                                float& shSt, float& shSl, float& shTim,
                                float periodSamples, float slewSamples)
        {
            switch (shape)
            {
                case Shape::Sine:
                    return fastSin (phase);

                case Shape::Triangle:
                {
                    constexpr float pi = 3.14159265f;
                    // Triangle: period 2*pi, peak at pi/2
                    float t = phase / (2.0f * pi);  // [0, 1)
                    return 1.0f - 4.0f * std::fabs (t - 0.5f);
                }

                case Shape::SampleAndHold:
                {
                    shTim += 1.0f;
                    if (shTim >= periodSamples)
                    {
                        shTim -= periodSamples;
                        // New random target in [-1, 1] — use sin to avoid PRNG dependency
                        shSt = fastSin (phase + shTim * 0.1f);
                    }
                    // Slew toward target
                    float diff = shSt - shSl;
                    float slewStep = diff / std::max (1.0f, slewSamples);
                    shSl += slewStep;
                    return shSl;
                }
            }
            return fastSin (phase);
        }
    } tide;

    //==========================================================================
    // Stage 4 — Reef (Householder FDN Early Reflections)
    //==========================================================================
    struct ReefStage
    {
        static constexpr int kNumChannels = 8;

        // Prime-number delay lengths at 48kHz (from spec)
        static constexpr int kPrimeLengths48k[kNumChannels] = {
            1153, 1627, 2053, 2503, 3079, 3637, 4093, 4799
        };

        struct DelayLine {
            std::vector<float> buffer;
            int writePos    = 0;
            float dampState = 0.0f;
        };
        std::array<DelayLine, kNumChannels> delays;

        // Allpass diffusers (2 per channel, in series)
        struct Allpass {
            std::vector<float> buffer;
            int writePos = 0;
            float g = 0.3f;
        };
        std::array<std::array<Allpass, 2>, kNumChannels> allpasses;

        // Pre-delay buffers (stereo)
        std::vector<float> predelayL;
        std::vector<float> predelayR;
        int predelayWriteL = 0;
        int predelayWriteR = 0;

        double sr = 44100.0;

        void prepare (double sampleRate, int /*maxBlockSize*/)
        {
            sr = sampleRate;
            float srScale = static_cast<float> (sampleRate) / 48000.0f;

            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                int len = static_cast<int> (kPrimeLengths48k[ch] * srScale) + 2;
                // Hard ceiling at 5000 samples as spec requires
                len = std::min (len, 5000);
                delays[ch].buffer.assign (len, 0.0f);
                delays[ch].writePos  = 0;
                delays[ch].dampState = 0.0f;

                // Allpass lengths: ~29ms and ~53ms at 44.1kHz, prime offsets
                static constexpr int kApLen[2] = { 557, 1031 };
                for (int a = 0; a < 2; ++a)
                {
                    int apLen = static_cast<int> (kApLen[a] * srScale) + 2;
                    allpasses[ch][a].buffer.assign (apLen, 0.0f);
                    allpasses[ch][a].writePos = 0;
                    allpasses[ch][a].g = 0.3f;
                }
            }

            // Pre-delay: max 50ms
            int pdLen = static_cast<int> (0.050 * sampleRate) + 4;
            predelayL.assign (pdLen, 0.0f);
            predelayR.assign (pdLen, 0.0f);
            predelayWriteL = predelayWriteR = 0;
        }

        void reset()
        {
            for (auto& dl : delays)
            {
                std::fill (dl.buffer.begin(), dl.buffer.end(), 0.0f);
                dl.writePos = 0; dl.dampState = 0.0f;
            }
            for (auto& row : allpasses)
                for (auto& ap : row)
                {
                    std::fill (ap.buffer.begin(), ap.buffer.end(), 0.0f);
                    ap.writePos = 0;
                }
            std::fill (predelayL.begin(), predelayL.end(), 0.0f);
            std::fill (predelayR.begin(), predelayR.end(), 0.0f);
            predelayWriteL = predelayWriteR = 0;
        }

        void processBlock (float* L, float* R, int numSamples,
                           float mix, float size, float decay,
                           float damping, float density,
                           float predelayMs, float width)
        {
            if (mix < 0.0001f) return;

            float srF   = static_cast<float> (sr);
            float srScale = srF / 48000.0f;

            // Clamp parameters
            decay    = std::min (decay,    0.99f);
            damping  = std::min (damping,  0.95f);
            density  = std::max (0.0f, std::min (density,  1.0f));
            float apCoeff = 0.1f + density * 0.5f;  // [0.1, 0.6]

            // Set allpass g per call
            for (auto& row : allpasses)
                for (auto& ap : row)
                    ap.g = apCoeff;

            // Damping coefficient: dampCoeff = 1 - damping * 0.7
            float dampCoeff = 1.0f - damping * 0.7f;

            // Pre-delay in samples
            int pdSamples = static_cast<int> (predelayMs * srF / 1000.0f);
            pdSamples = std::max (0, std::min (pdSamples, static_cast<int> (predelayL.size()) - 2));

            int pdSizeL = static_cast<int> (predelayL.size());
            int pdSizeR = static_cast<int> (predelayR.size());

            for (int i = 0; i < numSamples; ++i)
            {
                float inL = L[i];
                float inR = R[i];

                // Write and read pre-delay
                predelayL[predelayWriteL] = inL;
                predelayR[predelayWriteR] = inR;
                int pdReadL = (predelayWriteL - pdSamples + pdSizeL) % pdSizeL;
                int pdReadR = (predelayWriteR - pdSamples + pdSizeR) % pdSizeR;
                float pdL = predelayL[pdReadL];
                float pdR = predelayR[pdReadR];
                predelayWriteL = (predelayWriteL + 1) % pdSizeL;
                predelayWriteR = (predelayWriteR + 1) % pdSizeR;

                // Feed stereo into FDN channels: interleave L→channels 0,2,4,6 and R→1,3,5,7
                float fdnIn[kNumChannels];
                for (int ch = 0; ch < kNumChannels; ++ch)
                    fdnIn[ch] = (ch % 2 == 0) ? pdL : pdR;

                // Read from delay lines and apply LP damping
                float delayed[kNumChannels];
                for (int ch = 0; ch < kNumChannels; ++ch)
                {
                    int len = static_cast<int> (delays[ch].buffer.size());
                    // Pre-clamp effective delay to [1, len-1] before subtraction so the read
                    // position cannot escape the buffer at high sample rates (e.g. 192 kHz where
                    // srScale=4 would push the raw offset past the len*8 padding).
                    int effectiveDelay = juce::jlimit (1, len - 1,
                        static_cast<int> (kPrimeLengths48k[ch] * srScale * size * 2.7f + 1));
                    int rp = ((delays[ch].writePos - effectiveDelay) + len) % len;
                    float raw = delays[ch].buffer[rp];

                    // LP damping: state = state + dampCoeff * (in - state)
                    delays[ch].dampState = flushDenormal (
                        delays[ch].dampState + dampCoeff * (raw - delays[ch].dampState));
                    delayed[ch] = delays[ch].dampState;
                }

                // Householder mixing: out[i] = in[i] - (2/N) * sum(in)
                // For N=8: factor = 1/4
                float sum = 0.0f;
                for (int ch = 0; ch < kNumChannels; ++ch) sum += delayed[ch];
                float householderScale = 2.0f / static_cast<float> (kNumChannels);

                float mixed[kNumChannels];
                for (int ch = 0; ch < kNumChannels; ++ch)
                    mixed[ch] = delayed[ch] - householderScale * sum;

                // Write back: fdnIn[ch] + decay * mixed[ch]
                for (int ch = 0; ch < kNumChannels; ++ch)
                {
                    int len = static_cast<int> (delays[ch].buffer.size());
                    delays[ch].buffer[delays[ch].writePos] =
                        flushDenormal (fdnIn[ch] + decay * mixed[ch] + 1e-25f);
                    delays[ch].writePos = (delays[ch].writePos + 1) % len;
                }

                // Apply allpass diffusion per channel
                for (int ch = 0; ch < kNumChannels; ++ch)
                {
                    float sig = mixed[ch];
                    for (int a = 0; a < 2; ++a)
                    {
                        Allpass& ap = allpasses[ch][a];
                        int apLen = static_cast<int> (ap.buffer.size());
                        float apOut = ap.buffer[ap.writePos];
                        float w = sig - ap.g * apOut;
                        ap.buffer[ap.writePos] = flushDenormal (w);
                        ap.writePos = (ap.writePos + 1) % apLen;
                        sig = ap.g * w + apOut;
                    }
                    mixed[ch] = sig;
                }

                // Stereo output: even channels → L, odd channels → R
                float wetL = 0.0f, wetR = 0.0f;
                for (int ch = 0; ch < kNumChannels; ++ch)
                {
                    if (ch % 2 == 0) wetL += mixed[ch];
                    else             wetR += mixed[ch];
                }
                float normFDN = 1.0f / static_cast<float> (kNumChannels / 2);
                wetL *= normFDN;
                wetR *= normFDN;

                // M/S width
                float mid  = (wetL + wetR) * 0.5f;
                float side = (wetL - wetR) * 0.5f;
                wetL = mid + side * width;
                wetR = mid - side * width;

                L[i] = inL + mix * wetL;
                R[i] = inR + mix * wetR;
            }
        }
    } reef;

    //==========================================================================
    // Stage 5 — Surface (Cytomic SVF High-Shelf Sweep)
    //==========================================================================
    struct SurfaceStage
    {
        enum class Mode { HighShelf = 0, LowShelf, Bell };

        TPT_SVF svfL;
        TPT_SVF svfR;

        float freqSmoothed  = 4000.0f;
        float gainSmoothed  = 0.0f;
        float smoothCoeff   = 0.0f;

        double sr = 44100.0;

        void prepare (double sampleRate)
        {
            sr = sampleRate;
            // 20ms smooth time constant
            smoothCoeff = 1.0f - fastExp (-1.0f / (0.020f * static_cast<float> (sampleRate)));
            svfL.reset();
            svfR.reset();
        }

        void reset()
        {
            svfL.reset();
            svfR.reset();
            freqSmoothed = 4000.0f;
            gainSmoothed = 0.0f;
        }

        void processBlock (float* L, float* R, int numSamples,
                           float mix, float baseFreq, float gainDb,
                           float resonance, Mode mode,
                           float sweep, float sweepOctaves,
                           float coupling)
        {
            if (mix < 0.0001f) return;

            // COUPLING macro maps linearly to sweep position
            float effectiveSweep = sweep + coupling * (1.0f - sweep);
            effectiveSweep = std::max (0.0f, std::min (effectiveSweep, 1.0f));

            // Frequency from sweep position
            float targetFreq = baseFreq * fastPow2 ((effectiveSweep - 0.5f) * sweepOctaves);
            targetFreq = std::max (20.0f, std::min (targetFreq, static_cast<float> (sr) * 0.45f));

            float res = std::max (0.0f, std::min (resonance, 0.99f));

            for (int i = 0; i < numSamples; ++i)
            {
                // Smooth frequency and gain to prevent zipper noise
                freqSmoothed += smoothCoeff * (targetFreq  - freqSmoothed);
                gainSmoothed += smoothCoeff * (gainDb      - gainSmoothed);
                freqSmoothed  = flushDenormal (freqSmoothed);
                gainSmoothed  = flushDenormal (gainSmoothed);

                // Update SVF coefficients and gain every sample (they're smoothed)
                svfL.updateCoeffs (freqSmoothed, res, sr);
                svfR.updateCoeffs (freqSmoothed, res, sr);
                svfL.updateGain   (gainSmoothed);
                svfR.updateGain   (gainSmoothed);

                float inL = L[i];
                float inR = R[i];
                float wetL, wetR;

                switch (mode)
                {
                    case Mode::HighShelf:
                        wetL = svfL.processHighShelf (inL);
                        wetR = svfR.processHighShelf (inR);
                        break;
                    case Mode::LowShelf:
                        wetL = svfL.processLowShelf (inL);
                        wetR = svfR.processLowShelf (inR);
                        break;
                    case Mode::Bell:
                        wetL = svfL.processBell (inL, gainSmoothed);
                        wetR = svfR.processBell (inR, gainSmoothed);
                        break;
                    default:
                        wetL = inL; wetR = inR; break;
                }

                L[i] = inL + mix * (wetL - inL);
                R[i] = inR + mix * (wetR - inR);
            }
        }
    } surface;

    //==========================================================================
    // Stage 6 — Biolume (Shimmer Saturation)
    //==========================================================================
    struct BiolumeStage
    {
        enum class SatMode { Soft = 0, Hard, Fold };

        float envL = 0.0f;
        float envR = 0.0f;
        float attackCoeff  = 0.0f;
        float releaseCoeff = 0.0f;

        // 2-pole HP cascade states (L and R)
        float hpL1 = 0.0f, hpL2 = 0.0f;
        float hpR1 = 0.0f, hpR2 = 0.0f;
        float hpLpL1 = 0.0f, hpLpL2 = 0.0f;  // 1-pole LP states used for HP = x - LP
        float hpLpR1 = 0.0f, hpLpR2 = 0.0f;

        // Allpass decorrelation state (R shimmer channel)
        float apStateL = 0.0f;
        float apStateR = 0.0f;

        // 4-stage allpass network for Biolume spectral shimmer
        std::array<float, 4> apNetL = {};
        std::array<float, 4> apNetR = {};
        // Ring buffer for 2x playback speed pitch shifter
        std::vector<float> pitchBufL;
        std::vector<float> pitchBufR;
        int pitchWriteL = 0;
        int pitchWriteR = 0;
        float pitchReadL = 0.0f;
        float pitchReadR = 0.0f;

        double sr = 44100.0;

        void prepare (double sampleRate)
        {
            sr = sampleRate;
            attackCoeff  = 1.0f - fastExp (-1.0f / (0.001f  * static_cast<float> (sampleRate)));
            releaseCoeff = 1.0f - fastExp (-1.0f / (0.100f  * static_cast<float> (sampleRate)));
            envL = envR = 0.0f;
            hpL1 = hpL2 = hpR1 = hpR2 = 0.0f;
            hpLpL1 = hpLpL2 = hpLpR1 = hpLpR2 = 0.0f;
            apStateL = apStateR = 0.0f;
            apNetL.fill (0.0f);
            apNetR.fill (0.0f);

            // Pitch shifter ring buffer: ~50ms window at 2× speed
            int pbLen = static_cast<int> (0.050 * sampleRate) * 2 + 4;
            pitchBufL.assign (pbLen, 0.0f);
            pitchBufR.assign (pbLen, 0.0f);
            pitchWriteL = pitchWriteR = 0;
            pitchReadL  = pitchReadR  = 0.0f;
        }

        void reset()
        {
            envL = envR = 0.0f;
            hpL1 = hpL2 = hpR1 = hpR2 = 0.0f;
            hpLpL1 = hpLpL2 = hpLpR1 = hpLpR2 = 0.0f;
            apStateL = apStateR = 0.0f;
            apNetL.fill (0.0f); apNetR.fill (0.0f);
            std::fill (pitchBufL.begin(), pitchBufL.end(), 0.0f);
            std::fill (pitchBufR.begin(), pitchBufR.end(), 0.0f);
            pitchWriteL = pitchWriteR = 0;
            pitchReadL  = pitchReadR  = 0.0f;
        }

        void processBlock (float* L, float* R, int numSamples,
                           float mix, float thresholdDb, float drive,
                           float hpFreq, float shimmerBlend,
                           SatMode satMode, float spread,
                           float character)
        {
            if (mix < 0.0001f) return;

            float srF = static_cast<float> (sr);

            // CHARACTER macro scales drive and shimmer
            drive         = drive  * (0.1f + character * 0.9f);
            shimmerBlend  = shimmerBlend * (0.1f + character * 0.8f);

            // Clamp drive and shimmer to safe range
            shimmerBlend = std::min (shimmerBlend, 0.9f);

            // Pre-gain from drive: up to +24 dB
            float driveGain = fastPow2 (drive * 24.0f / 6.0205999f);

            // Threshold (linear)
            float threshLin = fastPow2 (thresholdDb / 6.0205999f);

            // HP filter coefficient (1-pole LP cascade for HP = x - LP)
            float hpCoeff = 1.0f - fastExp (-2.0f * 3.14159265f * hpFreq / srF);

            // Allpass g from spec: 0.7 for decorrelation
            constexpr float apG = 0.7f;
            // Allpass network coefficient (color control lives in caller, here we use apG)
            // 4 allpass stages, delay times: 7, 11, 17, 23 ms at 44.1kHz scaled
            // We implement as simple first-order allpass (Schroeder allpass)
            // delay line style: y = -g*x + x_delayed + g*y_delayed
            // Here we use 1-sample allpass (simplified for no extra heap)
            // y[n] = -g * x[n] + prev_x + g * prev_y
            // This gives a phase rotation without explicit delay line allocation.

            // Pitch shifter: 2× playback speed = octave up
            // Ring buffer write at 1×, read at 2× speed
            int pbSize = static_cast<int> (pitchBufL.size());

            for (int i = 0; i < numSamples; ++i)
            {
                float inL = L[i];
                float inR = R[i];

                // --- Level detection (peak, per channel) ---
                float absL = std::fabs (inL);
                float absR = std::fabs (inR);
                float coefL = (absL > envL) ? attackCoeff  : releaseCoeff;
                float coefR = (absR > envR) ? attackCoeff  : releaseCoeff;
                envL = flushDenormal (envL + coefL * (absL - envL));
                envR = flushDenormal (envR + coefR * (absR - envR));

                bool gateL = (envL > threshLin);
                bool gateR = (envR > threshLin);

                float shimL = 0.0f, shimR = 0.0f;

                if (gateL || gateR)
                {
                    // Full-wave rectification
                    float rectL = absL * driveGain;
                    float rectR = absR * driveGain;

                    // 2-pole HP filter on rectified signal (cascade of 1-pole HP)
                    // HP = x - LP
                    hpLpL1 = flushDenormal (hpLpL1 + hpCoeff * (rectL - hpLpL1));
                    hpLpL2 = flushDenormal (hpLpL2 + hpCoeff * (hpLpL1  - hpLpL2));
                    hpLpR1 = flushDenormal (hpLpR1 + hpCoeff * (rectR - hpLpR1));
                    hpLpR2 = flushDenormal (hpLpR2 + hpCoeff * (hpLpR1 - hpLpR2));
                    float hpOutL = rectL - hpLpL2;
                    float hpOutR = rectR - hpLpR2;

                    // Saturation
                    shimL = gateL ? applySaturation (hpOutL, satMode) : 0.0f;
                    shimR = gateR ? applySaturation (hpOutR, satMode) : 0.0f;

                    // 4-stage allpass network for diffuse tail (Schroeder 1-state form)
                    // v = x - g*state;  y = state + g*v;  state = v
                    float sigL = shimL, sigR = shimR;
                    for (int a = 0; a < 4; ++a)
                    {
                        float vL = sigL - apG * apNetL[a];
                        float vR = sigR - apG * apNetR[a];
                        sigL = apNetL[a] + apG * vL;
                        sigR = apNetR[a] + apG * vR;
                        apNetL[a] = flushDenormal (vL);
                        apNetR[a] = flushDenormal (vR);
                    }

                    // Write to pitch-shift ring buffer
                    pitchBufL[pitchWriteL] = sigL;
                    pitchBufR[pitchWriteR] = sigR;
                    pitchWriteL = (pitchWriteL + 1) % pbSize;
                    pitchWriteR = (pitchWriteR + 1) % pbSize;

                    // Read at 2× speed (octave up)
                    pitchReadL += 2.0f;
                    pitchReadR += 2.0f;
                    if (pitchReadL >= static_cast<float> (pbSize)) pitchReadL -= static_cast<float> (pbSize);
                    if (pitchReadR >= static_cast<float> (pbSize)) pitchReadR -= static_cast<float> (pbSize);

                    int rpL0 = static_cast<int> (pitchReadL) % pbSize;
                    int rpL1 = (rpL0 + 1) % pbSize;
                    float fracL = pitchReadL - static_cast<float> (static_cast<int> (pitchReadL));
                    shimL = pitchBufL[rpL0] * (1.0f - fracL) + pitchBufL[rpL1] * fracL;

                    int rpR0 = static_cast<int> (pitchReadR) % pbSize;
                    int rpR1 = (rpR0 + 1) % pbSize;
                    float fracR = pitchReadR - static_cast<float> (static_cast<int> (pitchReadR));
                    shimR = pitchBufR[rpR0] * (1.0f - fracR) + pitchBufR[rpR1] * fracR;

                    // Scale feedback by intensity (max 0.7 to prevent runaway)
                    shimL *= std::min (shimmerBlend, 0.7f);
                    shimR *= std::min (shimmerBlend, 0.7f);

                    // Stereo spread: allpass phase rotation on R
                    float apPrev = apStateR;
                    apStateR = flushDenormal (-apG * shimR + apStateL + apG * apStateR);
                    apStateL = shimR;
                    shimR = apG * shimR + apPrev;
                    shimR = shimR * spread + shimL * (1.0f - spread);
                }

                // Additive shimmer blend
                L[i] = inL * (1.0f - mix) + (inL + shimL) * mix;
                R[i] = inR * (1.0f - mix) + (inR + shimR) * mix;
            }
        }

    private:
        float applySaturation (float x, SatMode mode) const
        {
            switch (mode)
            {
                case SatMode::Soft:
                    return fastTanh (x);
                case SatMode::Hard:
                    return std::max (-1.0f, std::min (1.0f, x));
                case SatMode::Fold:
                    return foldback (x, 1.0f);
            }
            return fastTanh (x);
        }

        float foldback (float x, float ceiling) const
        {
            while (x >  ceiling) x = 2.0f * ceiling  - x;
            while (x < -ceiling) x = -2.0f * ceiling - x;
            return x;
        }
    } biolume;

    //==========================================================================
    // Cached parameter pointers
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
        std::atomic<float>* fathomChar     = nullptr;

        // Stage 2: Drift
        std::atomic<float>* driftMix       = nullptr;
        std::atomic<float>* driftVoices    = nullptr;
        std::atomic<float>* driftDepth     = nullptr;
        std::atomic<float>* driftRate      = nullptr;
        std::atomic<float>* driftDiffusion = nullptr;
        std::atomic<float>* driftFeedback  = nullptr;
        std::atomic<float>* driftSpread    = nullptr;
        std::atomic<float>* driftSpace     = nullptr;
        std::atomic<float>* driftMovement  = nullptr;

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
        std::atomic<float>* tideMovement   = nullptr;

        // Stage 4: Reef
        std::atomic<float>* reefMix        = nullptr;
        std::atomic<float>* reefSize       = nullptr;
        std::atomic<float>* reefDecay      = nullptr;
        std::atomic<float>* reefDamping    = nullptr;
        std::atomic<float>* reefDensity    = nullptr;
        std::atomic<float>* reefPredelay   = nullptr;
        std::atomic<float>* reefWidth      = nullptr;
        std::atomic<float>* reefSpace      = nullptr;

        // Stage 5: Surface
        std::atomic<float>* surfaceMix     = nullptr;
        std::atomic<float>* surfaceFreq    = nullptr;
        std::atomic<float>* surfaceGain    = nullptr;
        std::atomic<float>* surfaceRes     = nullptr;
        std::atomic<float>* surfaceMode    = nullptr;
        std::atomic<float>* surfaceSweep   = nullptr;
        std::atomic<float>* surfaceSweepRng= nullptr;
        std::atomic<float>* surfaceCoupling= nullptr;

        // Stage 6: Biolume
        std::atomic<float>* biolumeMix     = nullptr;
        std::atomic<float>* biolumeThresh  = nullptr;
        std::atomic<float>* biolumeDrive   = nullptr;
        std::atomic<float>* biolumeHpFreq  = nullptr;
        std::atomic<float>* biolumeShimmer = nullptr;
        std::atomic<float>* biolumeSatMode = nullptr;
        std::atomic<float>* biolumeSpread  = nullptr;
        std::atomic<float>* biolumeChar    = nullptr;
    } params;

    double sr        = 44100.0;
    int    blockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AquaticFXSuite)
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void AquaticFXSuite::prepare (double sampleRate, int samplesPerBlock)
{
    sr        = sampleRate;
    blockSize = samplesPerBlock;

    fathom.prepare (sampleRate);
    drift.prepare  (sampleRate);
    tide.prepare   (sampleRate);
    reef.prepare   (sampleRate, samplesPerBlock);
    surface.prepare(sampleRate);
    biolume.prepare(sampleRate);
}

inline void AquaticFXSuite::reset()
{
    fathom.reset();
    drift.reset();
    tide.reset();
    reef.reset();
    surface.reset();
    biolume.reset();
}

inline void AquaticFXSuite::processBlock (float* L, float* R, int numSamples, double bpm)
{
    //------------------------------------------------------------------
    // Stage 1 — Fathom
    //------------------------------------------------------------------
    {
        float mix      = params.fathomMix      ? params.fathomMix->load()      : 0.0f;
        float depth    = params.fathomDepth    ? params.fathomDepth->load()    : 0.5f;
        float lowGain  = params.fathomLowGain  ? params.fathomLowGain->load()  : 1.0f;
        float midGain  = params.fathomMidGain  ? params.fathomMidGain->load()  : 1.0f;
        float highGain = params.fathomHighGain ? params.fathomHighGain->load() : 1.0f;
        float lowXover = params.fathomLowXover ? params.fathomLowXover->load() : 200.0f;
        float hiXover  = params.fathomHiXover  ? params.fathomHiXover->load()  : 3000.0f;
        float character= params.fathomChar     ? params.fathomChar->load()     : 0.5f;

        fathom.processBlock (L, R, numSamples,
                             mix, depth, lowGain, midGain, highGain,
                             lowXover, hiXover, character);
    }

    //------------------------------------------------------------------
    // Stage 2 — Drift
    //------------------------------------------------------------------
    {
        float mix       = params.driftMix       ? params.driftMix->load()       : 0.0f;
        float voices    = params.driftVoices    ? params.driftVoices->load()    : 3.0f;
        float depth     = params.driftDepth     ? params.driftDepth->load()     : 8.0f;
        float feedback  = params.driftFeedback  ? params.driftFeedback->load()  : 0.0f;
        float spread    = params.driftSpread    ? params.driftSpread->load()    : 0.8f;
        float space     = params.driftSpace     ? params.driftSpace->load()     : 0.5f;
        float movement  = params.driftMovement  ? params.driftMovement->load()  : 0.5f;

        // SPACE macro scales stereo spread (0=mono, 1=maximum spread)
        float effectiveSpread = spread * space;
        // MOVEMENT macro overrides rate (0.001→2.0 Hz) and diffusion (0.05→0.80)
        float movRate = 0.001f * fastPow2 (movement * fastLog2 (2000.0f));
        float movDiff = 0.05f + movement * 0.75f;

        drift.processBlock (L, R, numSamples,
                            static_cast<int> (voices + 0.5f), depth, movRate,
                            movDiff, feedback, effectiveSpread, mix);
    }

    //------------------------------------------------------------------
    // Stage 3 — Tide
    //------------------------------------------------------------------
    {
        float mix        = params.tideMix        ? params.tideMix->load()        : 0.0f;
        float modeF      = params.tideMode       ? params.tideMode->load()       : 0.0f;
        float rate       = params.tideRate       ? params.tideRate->load()       : 0.5f;
        float depth      = params.tideDepth      ? params.tideDepth->load()      : 0.6f;
        float shapeF     = params.tideShape      ? params.tideShape->load()      : 0.0f;
        float syncEnF    = params.tideSyncEn     ? params.tideSyncEn->load()     : 0.0f;
        float syncDivF   = params.tideSyncDiv    ? params.tideSyncDiv->load()    : 3.0f;
        float filterFreq = params.tideFilterFreq ? params.tideFilterFreq->load() : 800.0f;
        float filterRes  = params.tideFilterRes  ? params.tideFilterRes->load()  : 0.3f;
        float stereoPhD  = params.tideStereoPhase? params.tideStereoPhase->load(): 90.0f;
        float movement   = params.tideMovement   ? params.tideMovement->load()   : 0.5f;

        // MOVEMENT macro: rate = 0.01 * 2^(movement * log2(800))
        rate = 0.01f * fastPow2 (movement * fastLog2 (800.0f));

        TideStage::Mode  mode  = static_cast<TideStage::Mode>  (static_cast<int> (modeF + 0.5f));
        TideStage::Shape shape = static_cast<TideStage::Shape> (static_cast<int> (shapeF + 0.5f));
        bool  syncEnabled = (syncEnF > 0.5f);
        int   syncDiv     = static_cast<int> (syncDivF + 0.5f);

        // Convert stereo phase offset degrees to radians
        float stereoPhaseRad = stereoPhD * (3.14159265f / 180.0f);

        // Set R channel phase from initial offset
        tide.lfoPhaseR = tide.lfoPhase + stereoPhaseRad;

        tide.processBlock (L, R, numSamples,
                           mix, mode, rate, depth, shape,
                           syncEnabled, syncDiv, bpm,
                           filterFreq, filterRes, stereoPhaseRad);
    }

    //------------------------------------------------------------------
    // Stage 4 — Reef
    //------------------------------------------------------------------
    {
        float mix      = params.reefMix      ? params.reefMix->load()      : 0.0f;
        float size     = params.reefSize     ? params.reefSize->load()     : 0.5f;
        float decay    = params.reefDecay    ? params.reefDecay->load()    : 0.4f;
        float damping  = params.reefDamping  ? params.reefDamping->load()  : 0.5f;
        float density  = params.reefDensity  ? params.reefDensity->load()  : 0.6f;
        float predelay = params.reefPredelay ? params.reefPredelay->load() : 8.0f;
        float width    = params.reefWidth    ? params.reefWidth->load()    : 0.8f;
        float space    = params.reefSpace    ? params.reefSpace->load()    : 0.5f;

        // SPACE macro morphs size (0.1→1.0) and decay (0.1→0.8)
        size  = 0.1f + space * 0.9f;
        decay = 0.1f + space * 0.7f;

        reef.processBlock (L, R, numSamples,
                           mix, size, decay, damping, density, predelay, width);
    }

    //------------------------------------------------------------------
    // Stage 5 — Surface
    //------------------------------------------------------------------
    {
        float mix        = params.surfaceMix      ? params.surfaceMix->load()      : 0.0f;
        float freq       = params.surfaceFreq     ? params.surfaceFreq->load()     : 4000.0f;
        float gain       = params.surfaceGain     ? params.surfaceGain->load()     : 0.0f;
        float res        = params.surfaceRes      ? params.surfaceRes->load()      : 0.3f;
        float modeF      = params.surfaceMode     ? params.surfaceMode->load()     : 0.0f;
        float sweep      = params.surfaceSweep    ? params.surfaceSweep->load()    : 0.5f;
        float sweepRngF  = params.surfaceSweepRng ? params.surfaceSweepRng->load() : 1.0f;
        float coupling   = params.surfaceCoupling ? params.surfaceCoupling->load() : 0.0f;

        SurfaceStage::Mode mode = static_cast<SurfaceStage::Mode> (static_cast<int> (modeF + 0.5f));
        // sweepOctaves: 0 = Narrow (1 oct), 1 = Wide (4 oct)
        float sweepOctaves = (sweepRngF > 0.5f) ? 4.0f : 1.0f;

        surface.processBlock (L, R, numSamples,
                              mix, freq, gain, res, mode,
                              sweep, sweepOctaves, coupling);
    }

    //------------------------------------------------------------------
    // Stage 6 — Biolume
    //------------------------------------------------------------------
    {
        float mix       = params.biolumeMix     ? params.biolumeMix->load()     : 0.0f;
        float thresh    = params.biolumeThresh  ? params.biolumeThresh->load()  : -18.0f;
        float drive     = params.biolumeDrive   ? params.biolumeDrive->load()   : 0.4f;
        float hpFreq    = params.biolumeHpFreq  ? params.biolumeHpFreq->load()  : 4000.0f;
        float shimmer   = params.biolumeShimmer ? params.biolumeShimmer->load() : 0.5f;
        float satModeF  = params.biolumeSatMode ? params.biolumeSatMode->load() : 0.0f;
        float spread    = params.biolumeSpread  ? params.biolumeSpread->load()  : 0.6f;
        float character = params.biolumeChar    ? params.biolumeChar->load()    : 0.5f;

        BiolumeStage::SatMode satMode =
            static_cast<BiolumeStage::SatMode> (static_cast<int> (satModeF + 0.5f));

        biolume.processBlock (L, R, numSamples,
                              mix, thresh, drive, hpFreq, shimmer,
                              satMode, spread, character);
    }
}

inline void AquaticFXSuite::cacheParameterPointers (juce::AudioProcessorValueTreeState& apvts)
{
    auto get = [&] (const char* id) -> std::atomic<float>* {
        auto* p = apvts.getRawParameterValue (id);
        return p;
    };

    // Fathom
    params.fathomMix      = get ("aqua_fathom_mix");
    params.fathomDepth    = get ("aqua_fathom_depth");
    params.fathomLowGain  = get ("aqua_fathom_lowGain");
    params.fathomMidGain  = get ("aqua_fathom_midGain");
    params.fathomHighGain = get ("aqua_fathom_highGain");
    params.fathomLowXover = get ("aqua_fathom_lowXover");
    params.fathomHiXover  = get ("aqua_fathom_highXover");
    params.fathomChar     = get ("aqua_fathom_character");

    // Drift
    params.driftMix       = get ("aqua_drift_mix");
    params.driftVoices    = get ("aqua_drift_voices");
    params.driftDepth     = get ("aqua_drift_depth");
    params.driftRate      = get ("aqua_drift_rate");
    params.driftDiffusion = get ("aqua_drift_diffusion");
    params.driftFeedback  = get ("aqua_drift_feedback");
    params.driftSpread    = get ("aqua_drift_stereoSpread");
    params.driftSpace     = get ("aqua_drift_space");
    params.driftMovement  = get ("aqua_drift_movement");

    // Tide
    params.tideMix        = get ("aqua_tide_mix");
    params.tideMode       = get ("aqua_tide_mode");
    params.tideRate       = get ("aqua_tide_rate");
    params.tideDepth      = get ("aqua_tide_depth");
    params.tideShape      = get ("aqua_tide_shape");
    params.tideSyncEn     = get ("aqua_tide_syncEnabled");
    params.tideSyncDiv    = get ("aqua_tide_syncDiv");
    params.tideFilterFreq = get ("aqua_tide_filterFreq");
    params.tideFilterRes  = get ("aqua_tide_filterRes");
    params.tideStereoPhase= get ("aqua_tide_stereoPhase");
    params.tideMovement   = get ("aqua_tide_movement");

    // Reef
    params.reefMix        = get ("aqua_reef_mix");
    params.reefSize       = get ("aqua_reef_size");
    params.reefDecay      = get ("aqua_reef_decay");
    params.reefDamping    = get ("aqua_reef_damping");
    params.reefDensity    = get ("aqua_reef_density");
    params.reefPredelay   = get ("aqua_reef_predelay");
    params.reefWidth      = get ("aqua_reef_width");
    params.reefSpace      = get ("aqua_reef_space");

    // Surface
    params.surfaceMix     = get ("aqua_surface_mix");
    params.surfaceFreq    = get ("aqua_surface_freq");
    params.surfaceGain    = get ("aqua_surface_gain");
    params.surfaceRes     = get ("aqua_surface_resonance");
    params.surfaceMode    = get ("aqua_surface_mode");
    params.surfaceSweep   = get ("aqua_surface_sweep");
    params.surfaceSweepRng= get ("aqua_surface_sweepRange");
    params.surfaceCoupling= get ("aqua_surface_coupling");

    // Biolume
    params.biolumeMix     = get ("aqua_biolume_mix");
    params.biolumeThresh  = get ("aqua_biolume_threshold");
    params.biolumeDrive   = get ("aqua_biolume_drive");
    params.biolumeHpFreq  = get ("aqua_biolume_hpFreq");
    params.biolumeShimmer = get ("aqua_biolume_shimmer");
    params.biolumeSatMode = get ("aqua_biolume_satMode");
    params.biolumeSpread  = get ("aqua_biolume_spread");
    params.biolumeChar    = get ("aqua_biolume_character");
}

inline void AquaticFXSuite::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using AP  = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    using NR  = juce::NormalisableRange<float>;

    //--- Stage 1: Fathom ---
    layout.add (std::make_unique<AP>  ("aqua_fathom_mix",       "Fathom Mix",       NR (0.0f,   1.0f, 0.001f),  0.0f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_depth",     "Fathom Depth",     NR (0.0f,   1.0f, 0.001f),  0.5f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_lowGain",   "Fathom Low Gain",  NR (0.0f,   2.0f, 0.001f),  1.0f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_midGain",   "Fathom Mid Gain",  NR (0.0f,   2.0f, 0.001f),  1.0f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_highGain",  "Fathom High Gain", NR (0.0f,   2.0f, 0.001f),  1.0f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_lowXover",  "Fathom Low Xover", NR (60.0f,  800.0f, 1.0f),  200.0f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_highXover", "Fathom Hi Xover",  NR (1000.0f,12000.0f,1.0f), 3000.0f));
    layout.add (std::make_unique<AP>  ("aqua_fathom_character", "Fathom Character", NR (0.0f,   1.0f, 0.001f),  0.5f));

    //--- Stage 2: Drift ---
    layout.add (std::make_unique<AP>  ("aqua_drift_mix",          "Drift Mix",          NR (0.0f,  1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<AP>  ("aqua_drift_voices",       "Drift Voices",       NR (2.0f,  6.0f, 1.0f),   3.0f));
    layout.add (std::make_unique<AP>  ("aqua_drift_depth",        "Drift Depth",        NR (0.0f, 40.0f, 0.01f),  8.0f));
    layout.add (std::make_unique<AP>  ("aqua_drift_rate",         "Drift Rate",         NR (0.001f, 2.0f, 0.001f),0.07f));
    layout.add (std::make_unique<AP>  ("aqua_drift_diffusion",    "Drift Diffusion",    NR (0.0f,  1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<AP>  ("aqua_drift_feedback",     "Drift Feedback",     NR (0.0f,  0.7f, 0.001f), 0.0f));
    layout.add (std::make_unique<AP>  ("aqua_drift_stereoSpread", "Drift Spread",       NR (0.0f,  1.0f, 0.001f), 0.8f));
    layout.add (std::make_unique<AP>  ("aqua_drift_space",        "Drift Space",        NR (0.0f,  1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<AP>  ("aqua_drift_movement",     "Drift Movement",     NR (0.0f,  1.0f, 0.001f), 0.5f));

    //--- Stage 3: Tide ---
    layout.add (std::make_unique<AP>  ("aqua_tide_mix",         "Tide Mix",         NR (0.0f,   1.0f,    0.001f), 0.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_mode",        "Tide Mode",        NR (0.0f,   2.0f,    1.0f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_rate",        "Tide Rate",        NR (0.01f,  8.0f,    0.001f), 0.5f));
    layout.add (std::make_unique<AP>  ("aqua_tide_depth",       "Tide Depth",       NR (0.0f,   1.0f,    0.001f), 0.6f));
    layout.add (std::make_unique<AP>  ("aqua_tide_shape",       "Tide Shape",       NR (0.0f,   2.0f,    1.0f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_syncEnabled", "Tide Sync Enable", NR (0.0f,   1.0f,    1.0f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_syncDiv",     "Tide Sync Div",    NR (0.0f,   7.0f,    1.0f),   3.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_filterFreq",  "Tide Filter Freq", NR (100.0f, 8000.0f, 1.0f),   800.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_filterRes",   "Tide Filter Res",  NR (0.0f,   0.95f,   0.001f), 0.3f));
    layout.add (std::make_unique<AP>  ("aqua_tide_stereoPhase", "Tide Stereo Phase",NR (0.0f, 180.0f,    0.1f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_tide_movement",    "Tide Movement",    NR (0.0f,   1.0f,    0.001f), 0.5f));

    //--- Stage 4: Reef ---
    layout.add (std::make_unique<AP>  ("aqua_reef_mix",      "Reef Mix",      NR (0.0f,  1.0f,    0.001f), 0.0f));
    layout.add (std::make_unique<AP>  ("aqua_reef_size",     "Reef Size",     NR (0.0f,  1.0f,    0.001f), 0.5f));
    layout.add (std::make_unique<AP>  ("aqua_reef_decay",    "Reef Decay",    NR (0.0f,  1.0f,    0.001f), 0.4f));
    layout.add (std::make_unique<AP>  ("aqua_reef_damping",  "Reef Damping",  NR (0.0f,  1.0f,    0.001f), 0.5f));
    layout.add (std::make_unique<AP>  ("aqua_reef_density",  "Reef Density",  NR (0.0f,  1.0f,    0.001f), 0.6f));
    layout.add (std::make_unique<AP>  ("aqua_reef_predelay", "Reef Predelay", NR (0.0f,  50.0f,   0.1f),   8.0f));
    layout.add (std::make_unique<AP>  ("aqua_reef_width",    "Reef Width",    NR (0.0f,  1.0f,    0.001f), 0.8f));
    layout.add (std::make_unique<AP>  ("aqua_reef_space",    "Reef Space",    NR (0.0f,  1.0f,    0.001f), 0.5f));

    //--- Stage 5: Surface ---
    layout.add (std::make_unique<AP>  ("aqua_surface_mix",        "Surface Mix",         NR (0.0f,     1.0f,     0.001f), 0.0f));
    layout.add (std::make_unique<AP>  ("aqua_surface_freq",       "Surface Freq",        NR (200.0f,16000.0f,    1.0f),   4000.0f));
    layout.add (std::make_unique<AP>  ("aqua_surface_gain",       "Surface Gain",        NR (-18.0f,  18.0f,     0.1f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_surface_resonance",  "Surface Resonance",   NR (0.0f,     1.0f,     0.001f), 0.3f));
    layout.add (std::make_unique<AP>  ("aqua_surface_mode",       "Surface Mode",        NR (0.0f,     2.0f,     1.0f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_surface_sweep",      "Surface Sweep",       NR (0.0f,     1.0f,     0.001f), 0.5f));
    layout.add (std::make_unique<AP>  ("aqua_surface_sweepRange", "Surface Sweep Range", NR (0.0f,     1.0f,     1.0f),   1.0f));
    layout.add (std::make_unique<AP>  ("aqua_surface_coupling",   "Surface Coupling",    NR (0.0f,     1.0f,     0.001f), 0.0f));

    //--- Stage 6: Biolume ---
    layout.add (std::make_unique<AP>  ("aqua_biolume_mix",       "Biolume Mix",       NR (0.0f,   1.0f,   0.001f), 0.0f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_threshold", "Biolume Threshold", NR (-60.0f, 0.0f,   0.1f),  -18.0f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_drive",     "Biolume Drive",     NR (0.0f,   1.0f,   0.001f), 0.4f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_hpFreq",    "Biolume HP Freq",   NR (2000.0f,12000.0f,1.0f), 4000.0f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_shimmer",   "Biolume Shimmer",   NR (0.0f,   1.0f,   0.001f), 0.5f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_satMode",   "Biolume Sat Mode",  NR (0.0f,   2.0f,   1.0f),   0.0f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_spread",    "Biolume Spread",    NR (0.0f,   1.0f,   0.001f), 0.6f));
    layout.add (std::make_unique<AP>  ("aqua_biolume_character", "Biolume Character", NR (0.0f,   1.0f,   0.001f), 0.5f));
}

} // namespace xomnibus
