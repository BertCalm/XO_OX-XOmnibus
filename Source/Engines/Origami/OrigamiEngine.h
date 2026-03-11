#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <complex>
#include <vector>

namespace xomnibus {

//==============================================================================
// OrigamiEngine — Spectral Folding FFT Synthesis.
//
// Transforms audio through a spectral origami process: sound enters the
// frequency domain via STFT, where 4 cascaded spectral operations (FOLD,
// MIRROR, ROTATE, STRETCH) sculpt the magnitude spectrum before resynthesis.
// The result is a class of timbres impossible through time-domain methods —
// spectra that fold back on themselves, creating dense metallic textures,
// crystalline harmonics, and otherworldly inharmonic tones.
//
// Features:
//   - Internal oscillator bank (saw + square + noise mix) + coupling input
//   - 2048-point STFT with 4x overlap (hop=512), Hann window
//   - 4 spectral operations: FOLD, MIRROR, ROTATE, STRETCH
//   - Phase vocoder with instantaneous frequency tracking
//   - Spectral freeze (hold current frame)
//   - 3-point triangular smoothing on folded magnitude
//   - Amp and Fold ADSR envelopes
//   - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//   - Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
//   - Full XOmnibus coupling support
//
// Coupling:
//   - Output: post-fold stereo audio via getSampleForCoupling
//   - Input: AudioToWavetable (replace/blend source), AmpToFilter (amp->fold depth),
//            EnvToMorph (envelope->fold point), RhythmToBlend (rhythm->freeze trigger)
//
//==============================================================================

//==============================================================================
// FFT constants.
//==============================================================================
static constexpr int kFFTSize     = 2048;
static constexpr int kFFTHalf     = kFFTSize / 2 + 1;       // 1025 bins
static constexpr int kHopSize     = kFFTSize / 4;            // 512 (4x overlap)
static constexpr int kOverlap     = 4;

//==============================================================================
// ADSR envelope generator — lightweight, inline, no allocation.
//==============================================================================
struct OrigamiADSR
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
struct OrigamiLFO
{
    enum class Shape { Sine, Triangle, Saw, Square, SandH };

    float phase = 0.0f;
    float phaseInc = 0.0f;
    Shape shape = Shape::Sine;
    float holdValue = 0.0f;
    uint32_t sampleCounter = 12345u;

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
// Spectral frame — holds magnitude/phase for one STFT analysis frame.
//==============================================================================
struct SpectralFrame
{
    std::array<float, kFFTHalf> magnitude {};
    std::array<float, kFFTHalf> phase {};
    std::array<float, kFFTHalf> prevPhase {};      // Phase from previous hop (for vocoder)
    std::array<float, kFFTHalf> instFreq {};       // Instantaneous frequency per bin

    void clear() noexcept
    {
        magnitude.fill (0.0f);
        phase.fill (0.0f);
        prevPhase.fill (0.0f);
        instFreq.fill (0.0f);
    }
};

//==============================================================================
// OrigamiVoice — per-voice state with STFT pipeline.
//==============================================================================
struct OrigamiVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // Oscillator state
    float sawPhase = 0.0f;
    float sqrPhase = 0.0f;
    float noiseState = 12345u;

    // Glide
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // Envelopes
    OrigamiADSR ampEnv;
    OrigamiADSR foldEnv;

    // LFOs (per-voice for free-running independence)
    OrigamiLFO lfo1;
    OrigamiLFO lfo2;

    // STFT analysis/resynthesis state
    std::array<float, kFFTSize> inputRing {};          // Input circular buffer
    std::array<float, kFFTSize> outputAccum {};         // Overlap-add accumulator
    int inputWritePos = 0;
    int hopCounter = 0;                                  // Samples until next hop

    // Spectral frames
    SpectralFrame analysisFrame;
    SpectralFrame frozenFrame;
    bool hasFrozenFrame = false;

    // STFT working buffers (pre-allocated)
    std::array<float, kFFTSize> windowedInput {};
    std::array<float, kFFTSize> fftReal {};
    std::array<float, kFFTSize> fftImag {};
    std::array<float, kFFTHalf> foldedMag {};
    std::array<float, kFFTHalf> foldedPhase {};
    std::array<float, kFFTSize> ifftReal {};
    std::array<float, kFFTSize> ifftImag {};
    std::array<float, kFFTSize> resynthWindow {};

    // Output filter (post-FFT smoothing)
    CytomicSVF postFilter;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        sawPhase = 0.0f;
        sqrPhase = 0.0f;
        noiseState = 12345u;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        inputWritePos = 0;
        hopCounter = 0;
        hasFrozenFrame = false;
        ampEnv.reset();
        foldEnv.reset();
        lfo1.reset();
        lfo2.reset();
        postFilter.reset();
        inputRing.fill (0.0f);
        outputAccum.fill (0.0f);
        analysisFrame.clear();
        frozenFrame.clear();
    }
};

//==============================================================================
// OrigamiEngine — the main engine class.
//==============================================================================
class OrigamiEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;
    static constexpr float kMagFloor = 1e-15f;   // Denormal protection floor

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Pre-compute smoothing coefficient (5ms time constant)
        smoothCoeff = 1.0f - std::exp (-kTwoPi * (1.0f / 0.005f) / srf);

        // Pre-compute crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Phase increment per bin for phase vocoder
        // Expected phase advance per hop = hopSize * 2*PI * binIndex / fftSize
        binFreqStep = srf / static_cast<float> (kFFTSize);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Coupling input buffer
        couplingInputBuffer.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Build Hann window
        for (int i = 0; i < kFFTSize; ++i)
        {
            hannWindow[static_cast<size_t> (i)] =
                0.5f * (1.0f - std::cos (kTwoPi * static_cast<float> (i) / static_cast<float> (kFFTSize)));
        }

        // Build bit-reversal table for FFT
        buildBitReversalTable();

        // Initialize voices
        for (auto& v : voices)
        {
            v.reset();
            v.postFilter.reset();
            v.postFilter.setMode (CytomicSVF::Mode::LowPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingFoldDepthMod = 0.0f;
        couplingFoldPointMod = 0.0f;
        couplingFreezeTrigger = 0.0f;
        couplingSourceMod = 0.0f;

        smoothedFoldPoint = 0.5f;
        smoothedFoldDepth = 0.5f;
        smoothedRotate = 0.0f;
        smoothedStretch = 0.0f;

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
        std::fill (couplingInputBuffer.begin(), couplingInputBuffer.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float pFoldPt       = loadParam (pFoldPoint, 0.5f);
        const float pFoldDp       = loadParam (pFoldDepth, 0.5f);
        const int   pFoldCt       = static_cast<int> (loadParam (pFoldCount, 1.0f));
        const int   pOp           = static_cast<int> (loadParam (pOperation, 0.0f));
        const float pRot          = loadParam (pRotate, 0.0f);
        const float pStr          = loadParam (pStretch, 0.0f);
        const float pFrz          = loadParam (pFreeze, 0.0f);
        const float pSrc          = loadParam (pSource, 0.0f);
        const float pOscMx        = loadParam (pOscMix, 0.5f);
        const float pLevelVal     = loadParam (pLevel, 0.8f);

        const float pAmpA         = loadParam (pAmpAttack, 0.01f);
        const float pAmpD         = loadParam (pAmpDecay, 0.1f);
        const float pAmpS         = loadParam (pAmpSustain, 0.8f);
        const float pAmpR         = loadParam (pAmpRelease, 0.3f);

        const float pFoldA        = loadParam (pFoldEnvAttack, 0.01f);
        const float pFoldD        = loadParam (pFoldEnvDecay, 0.2f);
        const float pFoldS        = loadParam (pFoldEnvSustain, 0.5f);
        const float pFoldR        = loadParam (pFoldEnvRelease, 0.3f);

        const float pLfo1R        = loadParam (pLfo1Rate, 1.0f);
        const float pLfo1D        = loadParam (pLfo1Depth, 0.0f);
        const int   pLfo1S        = static_cast<int> (loadParam (pLfo1Shape, 0.0f));
        const float pLfo2R        = loadParam (pLfo2Rate, 1.0f);
        const float pLfo2D        = loadParam (pLfo2Depth, 0.0f);
        const int   pLfo2S        = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        const int   voiceModeIdx  = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime     = loadParam (pGlide, 0.0f);

        const float macroChar     = loadParam (pMacroFold, 0.0f);
        const float macroMove     = loadParam (pMacroMotion, 0.0f);
        const float macroCoup     = loadParam (pMacroCoupling, 0.0f);
        const float macroSpace    = loadParam (pMacroSpace, 0.0f);

        // Determine max polyphony from voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;                        // Mono
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break;     // Legato
            case 2: maxPoly = 4; break;                                          // Poly4
            case 3: maxPoly = 8; break;                                          // Poly8
            default: maxPoly = 4; break;
        }

        // Glide coefficient
        float glideCoeff = 1.0f;
        if (glideTime > 0.001f)
            glideCoeff = 1.0f - std::exp (-1.0f / (glideTime * srf));

        // Apply macro and coupling offsets
        // M1 CHARACTER: foldPoint + foldDepth
        float effectiveFoldPoint = clamp (pFoldPt + macroChar * 0.4f + couplingFoldPointMod, 0.0f, 1.0f);
        float effectiveFoldDepth = clamp (pFoldDp + macroChar * 0.3f + couplingFoldDepthMod, 0.0f, 1.0f);
        // M2 MOVEMENT: rotate + LFO1->foldPoint modulation depth
        float effectiveRotate    = clamp (pRot + macroMove * 0.5f, -1.0f, 1.0f);
        float lfo1DepthMod       = pLfo1D + macroMove * 0.3f;
        // M3 COUPLING: source + coupling gain
        float effectiveSource    = clamp (pSrc + macroCoup * 0.5f + couplingSourceMod, 0.0f, 1.0f);
        // M4 SPACE: foldCount + stretch + reverb-like spectral smear
        int effectiveFoldCount   = std::max (1, std::min (4, pFoldCt + static_cast<int> (macroSpace * 2.0f)));
        float effectiveStretch   = clamp (pStr + macroSpace * 0.4f, -1.0f, 1.0f);

        // Freeze: combine parameter + coupling rhythm trigger
        bool freezeActive = (pFrz > 0.5f) || (couplingFreezeTrigger > 0.5f);

        // Reset coupling accumulators
        couplingFoldDepthMod = 0.0f;
        couplingFoldPointMod = 0.0f;
        couplingFreezeTrigger = 0.0f;
        couplingSourceMod = 0.0f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), maxPoly, monoMode, legatoMode, glideCoeff,
                        pAmpA, pAmpD, pAmpS, pAmpR, pFoldA, pFoldD, pFoldS, pFoldR,
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
            smoothedFoldPoint += (effectiveFoldPoint - smoothedFoldPoint) * smoothCoeff;
            smoothedFoldDepth += (effectiveFoldDepth - smoothedFoldDepth) * smoothCoeff;
            smoothedRotate    += (effectiveRotate    - smoothedRotate)    * smoothCoeff;
            smoothedStretch   += (effectiveStretch   - smoothedStretch)   * smoothCoeff;

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
                float ampLevel  = voice.ampEnv.process();
                float foldLevel = voice.foldEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * lfo1DepthMod;
                float lfo2Val = voice.lfo2.process() * pLfo2D;

                // LFO1 -> fold point, LFO2 -> rotate
                float modFoldPoint = clamp (smoothedFoldPoint + lfo1Val * 0.3f, 0.0f, 1.0f);
                float modRotate    = clamp (smoothedRotate + lfo2Val * 0.3f, -1.0f, 1.0f);

                // Fold depth modulated by fold envelope
                float modFoldDepth = smoothedFoldDepth * foldLevel;

                // --- Generate source signal ---
                float freq = voice.currentFreq;
                float phaseInc = freq / srf;

                // Saw oscillator (naive, anti-aliased by FFT process)
                voice.sawPhase += phaseInc;
                if (voice.sawPhase >= 1.0f) voice.sawPhase -= 1.0f;
                float saw = 2.0f * voice.sawPhase - 1.0f;

                // Square oscillator
                voice.sqrPhase += phaseInc;
                if (voice.sqrPhase >= 1.0f) voice.sqrPhase -= 1.0f;
                float sqr = (voice.sqrPhase < 0.5f) ? 1.0f : -1.0f;

                // Noise (LCG)
                voice.noiseState = voice.noiseState * 1664525u + 1013904223u;
                float noise = static_cast<float> (voice.noiseState & 0xFFFF) / 32768.0f - 1.0f;

                // Mix oscillators: pOscMx maps [0, 0.5] = saw->square, [0.5, 1.0] = square->noise
                float oscSample;
                if (pOscMx <= 0.5f)
                {
                    float t = pOscMx * 2.0f;
                    oscSample = saw * (1.0f - t) + sqr * t;
                }
                else
                {
                    float t = (pOscMx - 0.5f) * 2.0f;
                    oscSample = sqr * (1.0f - t) + noise * t;
                }

                // Blend with coupling input
                float couplingIn = 0.0f;
                if (sample < static_cast<int> (couplingInputBuffer.size()))
                    couplingIn = couplingInputBuffer[static_cast<size_t> (sample)];

                float sourceSample = oscSample * (1.0f - effectiveSource) + couplingIn * effectiveSource;

                // --- Feed into STFT input ring buffer ---
                voice.inputRing[static_cast<size_t> (voice.inputWritePos)] = sourceSample;
                voice.inputWritePos = (voice.inputWritePos + 1) % kFFTSize;

                // --- Advance hop counter and process STFT when ready ---
                voice.hopCounter++;
                if (voice.hopCounter >= kHopSize)
                {
                    voice.hopCounter = 0;
                    processSTFT (voice, modFoldPoint, modFoldDepth, effectiveFoldCount,
                                 pOp, modRotate, smoothedStretch, freezeActive);
                }

                // --- Read from overlap-add output buffer ---
                int readPos = (voice.inputWritePos) % kFFTSize;
                float outSample = voice.outputAccum[static_cast<size_t> (readPos)];
                // Clear the consumed sample from the accumulator
                voice.outputAccum[static_cast<size_t> (readPos)] = 0.0f;

                // Denormal protection
                outSample = flushDenormal (outSample);

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                outSample *= gain;

                // Slight stereo spread based on voice index (deterministic)
                float pan = 0.5f;
                if (kMaxVoices > 1)
                {
                    int voiceIdx = static_cast<int> (&voice - &voices[0]);
                    pan = 0.3f + 0.4f * static_cast<float> (voiceIdx) / static_cast<float> (kMaxVoices - 1);
                }
                float panL = std::cos (pan * kPI * 0.5f);
                float panR = std::sin (pan * kPI * 0.5f);

                mixL += outSample * panL;
                mixR += outSample * panR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // Apply master level
            float finalL = mixL * pLevelVal;
            float finalR = mixR * pLevelVal;

            // Denormal flush on output
            finalL = flushDenormal (finalL);
            finalR = flushDenormal (finalR);

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

        // Clear coupling input buffer for next block
        std::fill (couplingInputBuffer.begin(),
                   couplingInputBuffer.begin() + std::min (static_cast<size_t> (numSamples),
                                                            couplingInputBuffer.size()),
                   0.0f);

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
            case CouplingType::AudioToWavetable:
                // External audio replaces/blends with internal oscillator source
                if (sourceBuffer != nullptr)
                {
                    int count = std::min (numSamples, static_cast<int> (couplingInputBuffer.size()));
                    for (int i = 0; i < count; ++i)
                        couplingInputBuffer[static_cast<size_t> (i)] += sourceBuffer[i] * amount;
                }
                couplingSourceMod += amount * 0.3f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude modulates fold depth
                couplingFoldDepthMod += amount * 0.5f;
                break;

            case CouplingType::EnvToMorph:
                // External envelope modulates fold point
                couplingFoldPointMod += amount * 0.4f;
                break;

            case CouplingType::RhythmToBlend:
                // Rhythm pattern triggers spectral freeze
                if (amount > 0.5f)
                    couplingFreezeTrigger = 1.0f;
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
        // --- Core Spectral Fold Parameters ---
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

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_level", 1 }, "Origami Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
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

        // --- Fold Envelope ---
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

        // --- LFO 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo1Rate", 1 }, "Origami LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo1Depth", 1 }, "Origami LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_lfo1Shape", 1 }, "Origami LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo2Rate", 1 }, "Origami LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_lfo2Depth", 1 }, "Origami LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_lfo2Shape", 1 }, "Origami LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Voice Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "origami_voiceMode", 1 }, "Origami Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "origami_glide", 1 }, "Origami Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // --- Macros ---
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

        pAmpAttack         = apvts.getRawParameterValue ("origami_ampAttack");
        pAmpDecay          = apvts.getRawParameterValue ("origami_ampDecay");
        pAmpSustain        = apvts.getRawParameterValue ("origami_ampSustain");
        pAmpRelease        = apvts.getRawParameterValue ("origami_ampRelease");

        pFoldEnvAttack     = apvts.getRawParameterValue ("origami_foldEnvAttack");
        pFoldEnvDecay      = apvts.getRawParameterValue ("origami_foldEnvDecay");
        pFoldEnvSustain    = apvts.getRawParameterValue ("origami_foldEnvSustain");
        pFoldEnvRelease    = apvts.getRawParameterValue ("origami_foldEnvRelease");

        pLfo1Rate          = apvts.getRawParameterValue ("origami_lfo1Rate");
        pLfo1Depth         = apvts.getRawParameterValue ("origami_lfo1Depth");
        pLfo1Shape         = apvts.getRawParameterValue ("origami_lfo1Shape");
        pLfo2Rate          = apvts.getRawParameterValue ("origami_lfo2Rate");
        pLfo2Depth         = apvts.getRawParameterValue ("origami_lfo2Depth");
        pLfo2Shape         = apvts.getRawParameterValue ("origami_lfo2Shape");

        pVoiceMode         = apvts.getRawParameterValue ("origami_voiceMode");
        pGlide             = apvts.getRawParameterValue ("origami_glide");

        pMacroFold         = apvts.getRawParameterValue ("origami_macroFold");
        pMacroMotion       = apvts.getRawParameterValue ("origami_macroMotion");
        pMacroCoupling     = apvts.getRawParameterValue ("origami_macroCoupling");
        pMacroSpace        = apvts.getRawParameterValue ("origami_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Origami"; }

    juce::Colour getAccentColour() const override { return juce::Colour (0xFFE63946); }

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
    // FFT — Radix-2 Decimation-In-Time, in-place.
    //==========================================================================

    void buildBitReversalTable()
    {
        int n = kFFTSize;
        int log2n = 0;
        {
            int temp = n;
            while (temp > 1) { temp >>= 1; ++log2n; }
        }

        for (int i = 0; i < n; ++i)
        {
            int reversed = 0;
            int val = i;
            for (int j = 0; j < log2n; ++j)
            {
                reversed = (reversed << 1) | (val & 1);
                val >>= 1;
            }
            bitReversalTable[static_cast<size_t> (i)] = reversed;
        }

        // Pre-compute twiddle factors
        for (int i = 0; i < kFFTSize / 2; ++i)
        {
            float angle = -kTwoPi * static_cast<float> (i) / static_cast<float> (kFFTSize);
            twiddleReal[static_cast<size_t> (i)] = std::cos (angle);
            twiddleImag[static_cast<size_t> (i)] = std::sin (angle);
        }
    }

    // Forward FFT: time domain -> frequency domain
    void fftForward (float* re, float* im) const noexcept
    {
        int n = kFFTSize;

        // Bit-reversal permutation
        for (int i = 0; i < n; ++i)
        {
            int j = bitReversalTable[static_cast<size_t> (i)];
            if (j > i)
            {
                std::swap (re[i], re[j]);
                std::swap (im[i], im[j]);
            }
        }

        // Butterfly stages
        for (int size = 2; size <= n; size *= 2)
        {
            int halfSize = size / 2;
            int step = n / size;

            for (int i = 0; i < n; i += size)
            {
                for (int k = 0; k < halfSize; ++k)
                {
                    int twIdx = k * step;
                    float wr = twiddleReal[static_cast<size_t> (twIdx)];
                    float wi = twiddleImag[static_cast<size_t> (twIdx)];

                    int even = i + k;
                    int odd  = i + k + halfSize;

                    float tr = re[odd] * wr - im[odd] * wi;
                    float ti = re[odd] * wi + im[odd] * wr;

                    re[odd] = re[even] - tr;
                    im[odd] = im[even] - ti;
                    re[even] += tr;
                    im[even] += ti;
                }
            }
        }
    }

    // Inverse FFT: frequency domain -> time domain
    void fftInverse (float* re, float* im) const noexcept
    {
        int n = kFFTSize;

        // Conjugate input
        for (int i = 0; i < n; ++i)
            im[i] = -im[i];

        // Forward FFT
        fftForward (re, im);

        // Conjugate and scale
        float invN = 1.0f / static_cast<float> (n);
        for (int i = 0; i < n; ++i)
        {
            re[i] *= invN;
            im[i] = -im[i] * invN;
        }
    }

    //==========================================================================
    // STFT Processing — the heart of spectral folding.
    //==========================================================================

    void processSTFT (OrigamiVoice& voice, float foldPoint, float foldDepth,
                      int foldCount, int operation, float rotate, float stretch,
                      bool freezeActive) noexcept
    {
        // --- Step 1: Window the input ---
        // Read kFFTSize samples ending at the current write position
        for (int i = 0; i < kFFTSize; ++i)
        {
            int readIdx = (voice.inputWritePos - kFFTSize + i + kFFTSize) % kFFTSize;
            voice.windowedInput[static_cast<size_t> (i)] =
                voice.inputRing[static_cast<size_t> (readIdx)] * hannWindow[static_cast<size_t> (i)];
        }

        // --- Step 2: FFT ---
        // Copy windowed input to FFT buffers
        for (int i = 0; i < kFFTSize; ++i)
        {
            voice.fftReal[static_cast<size_t> (i)] = voice.windowedInput[static_cast<size_t> (i)];
            voice.fftImag[static_cast<size_t> (i)] = 0.0f;
        }

        fftForward (voice.fftReal.data(), voice.fftImag.data());

        // --- Step 3: Extract magnitude and phase ---
        if (!freezeActive)
        {
            // Save previous phase for phase vocoder
            voice.analysisFrame.prevPhase = voice.analysisFrame.phase;

            for (int bin = 0; bin < kFFTHalf; ++bin)
            {
                float re = voice.fftReal[static_cast<size_t> (bin)];
                float im = voice.fftImag[static_cast<size_t> (bin)];

                float mag = std::sqrt (re * re + im * im);
                mag = std::max (mag, kMagFloor);    // Denormal protection

                float ph = std::atan2 (im, re);

                voice.analysisFrame.magnitude[static_cast<size_t> (bin)] = mag;
                voice.analysisFrame.phase[static_cast<size_t> (bin)] = ph;

                // Phase vocoder: compute instantaneous frequency
                float phaseDiff = ph - voice.analysisFrame.prevPhase[static_cast<size_t> (bin)];
                // Expected phase advance for this bin
                float expected = kTwoPi * static_cast<float> (bin) * static_cast<float> (kHopSize)
                                 / static_cast<float> (kFFTSize);
                // Deviation from expected
                float deviation = phaseDiff - expected;
                // Wrap to [-pi, pi]
                deviation = deviation - kTwoPi * std::round (deviation / kTwoPi);
                // Instantaneous frequency for this bin
                voice.analysisFrame.instFreq[static_cast<size_t> (bin)] =
                    static_cast<float> (bin) * binFreqStep + deviation * srf / (kTwoPi * static_cast<float> (kHopSize));
            }

            // Store frozen frame for potential freeze
            voice.frozenFrame = voice.analysisFrame;
            voice.hasFrozenFrame = true;
        }

        // Use frozen frame if freeze is active
        const SpectralFrame& sourceFrame = (freezeActive && voice.hasFrozenFrame)
                                            ? voice.frozenFrame
                                            : voice.analysisFrame;

        // --- Step 4: Apply spectral fold operations ---
        // Start with a copy of the analysis magnitude/phase
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            voice.foldedMag[static_cast<size_t> (bin)] = sourceFrame.magnitude[static_cast<size_t> (bin)];
            voice.foldedPhase[static_cast<size_t> (bin)] = sourceFrame.phase[static_cast<size_t> (bin)];
        }

        // Apply the selected operation, cascaded foldCount times
        for (int fold = 0; fold < foldCount; ++fold)
        {
            // Shift fold point slightly per cascade for richer results
            float cascadedFoldPoint = clamp (foldPoint + static_cast<float> (fold) * 0.1f, 0.0f, 0.99f);

            switch (operation)
            {
                case 0:  // FOLD — reflect spectrum at fold point
                    applySpectralFold (voice.foldedMag.data(), voice.foldedPhase.data(),
                                       cascadedFoldPoint, foldDepth);
                    break;
                case 1:  // MIRROR — bilateral symmetry around fold point
                    applySpectralMirror (voice.foldedMag.data(), voice.foldedPhase.data(),
                                         cascadedFoldPoint, foldDepth);
                    break;
                case 2:  // ROTATE — circular shift of magnitude
                    applySpectralRotate (voice.foldedMag.data(), voice.foldedPhase.data(),
                                         rotate, foldDepth);
                    break;
                case 3:  // STRETCH — nonlinear frequency-axis warping
                    applySpectralStretch (voice.foldedMag.data(), voice.foldedPhase.data(),
                                          stretch, foldDepth);
                    break;
                default:
                    break;
            }
        }

        // --- Step 5: 3-point triangular smoothing on folded magnitude ---
        applyTriangularSmoothing (voice.foldedMag.data());

        // --- Step 6: Reconstruct complex spectrum from modified magnitude + phase ---
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            float mag = std::max (voice.foldedMag[static_cast<size_t> (bin)], kMagFloor);
            float ph  = voice.foldedPhase[static_cast<size_t> (bin)];

            voice.ifftReal[static_cast<size_t> (bin)] = mag * std::cos (ph);
            voice.ifftImag[static_cast<size_t> (bin)] = mag * std::sin (ph);
        }

        // Mirror conjugate for negative frequencies
        for (int bin = 1; bin < kFFTSize / 2; ++bin)
        {
            int mirror = kFFTSize - bin;
            voice.ifftReal[static_cast<size_t> (mirror)] =  voice.ifftReal[static_cast<size_t> (bin)];
            voice.ifftImag[static_cast<size_t> (mirror)] = -voice.ifftImag[static_cast<size_t> (bin)];
        }

        // --- Step 7: Inverse FFT ---
        fftInverse (voice.ifftReal.data(), voice.ifftImag.data());

        // --- Step 8: Window and overlap-add ---
        // Normalization factor for 4x overlap with Hann window
        // Sum of Hann^2 across overlaps = 1.5, so scale by 2/3
        constexpr float overlapNorm = 2.0f / 3.0f;

        for (int i = 0; i < kFFTSize; ++i)
        {
            float sample = voice.ifftReal[static_cast<size_t> (i)]
                         * hannWindow[static_cast<size_t> (i)]
                         * overlapNorm;

            // Add to overlap accumulator at the correct position
            int outPos = (voice.inputWritePos + i) % kFFTSize;
            voice.outputAccum[static_cast<size_t> (outPos)] += flushDenormal (sample);
        }
    }

    //==========================================================================
    // Spectral Operations
    //==========================================================================

    // FOLD: Reflect spectrum at the fold point.
    // Bins above the fold point are reflected back below it, adding to existing content.
    void applySpectralFold (float* mag, float* ph, float foldPoint, float depth) const noexcept
    {
        int foldBin = std::max (1, static_cast<int> (foldPoint * static_cast<float> (kFFTHalf - 1)));

        // Work with a temporary copy for the folded content
        std::array<float, kFFTHalf> tempMag {};
        std::array<float, kFFTHalf> tempPh {};

        // Copy original below fold point
        for (int bin = 0; bin < foldBin && bin < kFFTHalf; ++bin)
        {
            tempMag[static_cast<size_t> (bin)] = mag[bin];
            tempPh[static_cast<size_t> (bin)]  = ph[bin];
        }

        // Reflect bins above fold point back below it
        for (int bin = foldBin; bin < kFFTHalf; ++bin)
        {
            int distance = bin - foldBin;
            int reflectedBin = foldBin - 1 - distance;

            if (reflectedBin >= 0 && reflectedBin < kFFTHalf)
            {
                // Add reflected magnitude (constructive interference)
                tempMag[static_cast<size_t> (reflectedBin)] += mag[bin] * depth;
                // Average the phases for smoother result
                tempPh[static_cast<size_t> (reflectedBin)] =
                    tempPh[static_cast<size_t> (reflectedBin)] * 0.7f + ph[bin] * 0.3f;
            }
        }

        // Zero out above fold point (all energy is now below)
        for (int bin = foldBin; bin < kFFTHalf; ++bin)
        {
            tempMag[static_cast<size_t> (bin)] = mag[bin] * (1.0f - depth);
            tempPh[static_cast<size_t> (bin)]  = ph[bin];
        }

        // Write back blended with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            mag[bin] = lerp (mag[bin], tempMag[static_cast<size_t> (bin)], depth);
            ph[bin]  = tempPh[static_cast<size_t> (bin)];
        }
    }

    // MIRROR: Create bilateral symmetry around the fold point.
    // Replaces spectrum with a version mirrored around the fold point.
    void applySpectralMirror (float* mag, float* ph, float foldPoint, float depth) const noexcept
    {
        int foldBin = std::max (1, static_cast<int> (foldPoint * static_cast<float> (kFFTHalf - 1)));

        std::array<float, kFFTHalf> tempMag {};
        std::array<float, kFFTHalf> tempPh {};

        // Below fold point: keep original
        for (int bin = 0; bin < foldBin && bin < kFFTHalf; ++bin)
        {
            tempMag[static_cast<size_t> (bin)] = mag[bin];
            tempPh[static_cast<size_t> (bin)]  = ph[bin];
        }

        // Above fold point: mirror from below
        for (int bin = foldBin; bin < kFFTHalf; ++bin)
        {
            int distance = bin - foldBin;
            int mirrorBin = foldBin - 1 - distance;

            if (mirrorBin >= 0 && mirrorBin < kFFTHalf)
            {
                tempMag[static_cast<size_t> (bin)] = mag[static_cast<size_t> (mirrorBin)];
                // Invert phase for the mirrored content (creates interesting interference)
                tempPh[static_cast<size_t> (bin)]  = -ph[static_cast<size_t> (mirrorBin)];
            }
            else
            {
                tempMag[static_cast<size_t> (bin)] = kMagFloor;
                tempPh[static_cast<size_t> (bin)]  = 0.0f;
            }
        }

        // Blend with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            mag[bin] = lerp (mag[bin], tempMag[static_cast<size_t> (bin)], depth);
            ph[bin]  = lerp (ph[bin],  tempPh[static_cast<size_t> (bin)],  depth);
        }
    }

    // ROTATE: Circular shift of magnitude spectrum.
    // Shifts all bins by a frequency-proportional amount.
    void applySpectralRotate (float* mag, float* ph, float rotateAmount, float depth) const noexcept
    {
        // Map [-1, 1] to bin shift range [-kFFTHalf/2, +kFFTHalf/2]
        float shift = rotateAmount * static_cast<float> (kFFTHalf / 2);

        std::array<float, kFFTHalf> tempMag {};
        std::array<float, kFFTHalf> tempPh {};
        tempMag.fill (kMagFloor);
        tempPh.fill (0.0f);

        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            float newBinF = static_cast<float> (bin) + shift;
            // Circular wrap
            while (newBinF < 0.0f) newBinF += static_cast<float> (kFFTHalf);
            while (newBinF >= static_cast<float> (kFFTHalf)) newBinF -= static_cast<float> (kFFTHalf);

            int newBin0 = static_cast<int> (newBinF);
            int newBin1 = (newBin0 + 1) % kFFTHalf;
            float frac = newBinF - static_cast<float> (newBin0);

            if (newBin0 >= 0 && newBin0 < kFFTHalf)
            {
                tempMag[static_cast<size_t> (newBin0)] += mag[bin] * (1.0f - frac);
                tempPh[static_cast<size_t> (newBin0)]   = ph[bin];
            }
            if (newBin1 >= 0 && newBin1 < kFFTHalf)
            {
                tempMag[static_cast<size_t> (newBin1)] += mag[bin] * frac;
                if (tempMag[static_cast<size_t> (newBin1)] < mag[bin] * frac * 1.1f)
                    tempPh[static_cast<size_t> (newBin1)] = ph[bin];
            }
        }

        // Blend with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            mag[bin] = lerp (mag[bin], std::max (tempMag[static_cast<size_t> (bin)], kMagFloor), depth);
            ph[bin]  = lerp (ph[bin],  tempPh[static_cast<size_t> (bin)], depth);
        }
    }

    // STRETCH: Nonlinear frequency-axis warping.
    // Positive stretch expands lower frequencies (darker), negative expands upper (brighter).
    void applySpectralStretch (float* mag, float* ph, float stretchAmount, float depth) const noexcept
    {
        std::array<float, kFFTHalf> tempMag {};
        std::array<float, kFFTHalf> tempPh {};
        tempMag.fill (kMagFloor);
        tempPh.fill (0.0f);

        // Map stretch [-1, 1] to warping exponent [0.5, 2.0]
        // stretch = 0 -> exponent = 1.0 (no change)
        // stretch > 0 -> exponent > 1.0 (compress upper, expand lower -> darker)
        // stretch < 0 -> exponent < 1.0 (compress lower, expand upper -> brighter)
        float exponent = std::pow (2.0f, stretchAmount);

        for (int bin = 1; bin < kFFTHalf; ++bin)
        {
            // Normalized frequency [0, 1]
            float normFreq = static_cast<float> (bin) / static_cast<float> (kFFTHalf - 1);
            // Apply nonlinear warp
            float warpedFreq = std::pow (normFreq, exponent);
            // Map back to bin index
            float warpedBinF = warpedFreq * static_cast<float> (kFFTHalf - 1);

            int wBin0 = static_cast<int> (warpedBinF);
            int wBin1 = wBin0 + 1;
            float frac = warpedBinF - static_cast<float> (wBin0);

            if (wBin0 >= 0 && wBin0 < kFFTHalf)
            {
                tempMag[static_cast<size_t> (wBin0)] += mag[bin] * (1.0f - frac);
                tempPh[static_cast<size_t> (wBin0)]   = ph[bin];
            }
            if (wBin1 >= 0 && wBin1 < kFFTHalf)
            {
                tempMag[static_cast<size_t> (wBin1)] += mag[bin] * frac;
                tempPh[static_cast<size_t> (wBin1)]   = ph[bin];
            }
        }

        // Preserve DC
        tempMag[0] = mag[0];
        tempPh[0]  = ph[0];

        // Blend with depth
        for (int bin = 0; bin < kFFTHalf; ++bin)
        {
            mag[bin] = lerp (mag[bin], std::max (tempMag[static_cast<size_t> (bin)], kMagFloor), depth);
            ph[bin]  = lerp (ph[bin],  tempPh[static_cast<size_t> (bin)], depth);
        }
    }

    //==========================================================================
    // 3-point triangular smoothing on magnitude spectrum.
    //==========================================================================

    static void applyTriangularSmoothing (float* mag) noexcept
    {
        // Weights: [0.25, 0.5, 0.25] — standard 3-point triangular kernel
        float prev = mag[0];
        for (int bin = 1; bin < kFFTHalf - 1; ++bin)
        {
            float curr = mag[bin];
            float smoothed = prev * 0.25f + curr * 0.5f + mag[bin + 1] * 0.25f;
            mag[bin] = std::max (smoothed, kMagFloor);
            prev = curr;
        }
    }

    //==========================================================================
    // MIDI note handling.
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int maxPoly,
                 bool monoMode, bool legatoMode, float glideCoeff,
                 float ampA, float ampD, float ampS, float ampR,
                 float foldA, float foldD, float foldS, float foldR,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape)
    {
        float freq = midiToHz (static_cast<float> (noteNumber));

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;

            if (legatoMode && wasActive)
            {
                // Legato: don't retrigger envelopes, just glide
                voice.glideCoeff = glideCoeff;
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
                voice.glideCoeff = glideCoeff;
                voice.sawPhase = 0.0f;
                voice.sqrPhase = 0.0f;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.foldEnv.setParams (foldA, foldD, foldS, foldR, srf);
                voice.foldEnv.noteOn();

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
        voice.sawPhase = 0.0f;
        voice.sqrPhase = 0.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.hopCounter = 0;
        voice.hasFrozenFrame = false;

        // Clear STFT buffers for clean start
        voice.inputRing.fill (0.0f);
        voice.outputAccum.fill (0.0f);
        voice.analysisFrame.clear();

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.foldEnv.setParams (foldA, foldD, foldS, foldR, srf);
        voice.foldEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate (lfo2Rate, srf);
        voice.lfo2.setShape (lfo2Shape);
        voice.lfo2.reset();

        voice.postFilter.reset();
        voice.postFilter.setMode (CytomicSVF::Mode::LowPass);
        voice.postFilter.setCoefficients (18000.0f, 0.1f, srf);
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.ampEnv.noteOff();
                voice.foldEnv.noteOff();
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

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float smoothCoeff = 0.1f;
    float crossfadeRate = 0.01f;
    float binFreqStep = 44100.0f / static_cast<float> (kFFTSize);

    std::array<OrigamiVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    int activeVoices = 0;

    // Pre-computed Hann window
    std::array<float, kFFTSize> hannWindow {};

    // Pre-computed bit-reversal table and twiddle factors for FFT
    std::array<int, kFFTSize> bitReversalTable {};
    std::array<float, kFFTSize / 2> twiddleReal {};
    std::array<float, kFFTSize / 2> twiddleImag {};

    // Smoothed control parameters
    float smoothedFoldPoint = 0.5f;
    float smoothedFoldDepth = 0.5f;
    float smoothedRotate = 0.0f;
    float smoothedStretch = 0.0f;

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingFoldDepthMod = 0.0f;
    float couplingFoldPointMod = 0.0f;
    float couplingFreezeTrigger = 0.0f;
    float couplingSourceMod = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Coupling input audio buffer
    std::vector<float> couplingInputBuffer;

    // Cached APVTS parameter pointers
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

    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    std::atomic<float>* pFoldEnvAttack = nullptr;
    std::atomic<float>* pFoldEnvDecay = nullptr;
    std::atomic<float>* pFoldEnvSustain = nullptr;
    std::atomic<float>* pFoldEnvRelease = nullptr;

    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;

    std::atomic<float>* pMacroFold = nullptr;
    std::atomic<float>* pMacroMotion = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;
};

} // namespace xomnibus
