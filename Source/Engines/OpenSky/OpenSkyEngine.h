// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OpenSkyEngine.h — XOpenSky | "The Soaring High"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOpenSky is the sunlit surface — the place where light breaks through
//      water and scatters into a million prismatic shards. It is the moment
//      of ascension, the upward rush, the euphoric breath after a deep dive.
//      Pure feliX polarity: bright, optimistic, transcendent.
//
//      In the XO_OX aquatic mythology, OpenSky is the flying fish — the
//      creature that defies its element, leaping from water into air, wings
//      spread against the sun. Where XOceanDeep is the abyss below, OpenSky
//      is the sky above. Together they are "The Full Column" — the entire
//      mythological space from trench to stratosphere.
//
//  ENGINE CONCEPT:
//      A euphoric shimmer synth built for supersaw anthems, crystalline pads,
//      and soaring leads. The DSP chain:
//
//          Supersaw Stack (7 detuned saws, PolyBLEP anti-aliased)
//            -> Bright Filter (2-pole SVF: LP + HP for tonal shaping)
//            -> Shimmer Stage (pitch-shifted reverb: octave-up + fifth-up)
//            -> Stereo Chorus (3-voice slow LFO modulation)
//            -> Unison Engine (up to 7 unison voices, detune + stereo spread)
//            -> Amp Envelope -> Output
//
//  4 MACROS:
//      RISE    — Ascension: pitch envelope up + filter opens + shimmer increases
//      WIDTH   — Stereo expansion: chorus depth + unison spread
//      GLOW    — Shimmer tail length + feedback amount
//      AIR     — Reverb size + high-frequency content boost
//
//  ACCENT COLOR: Sunburst #FF8C00
//  PARAMETER PREFIX: sky_
//  GALLERY CODE: OPENSKY
//
//  COUPLING:
//      Output: post-process stereo audio, envelope level
//      Input:  AmpToFilter (modulate filter cutoff)
//              LFOToPitch  (pitch drift from other engines)
//              AudioToFM   (FM modulation on supersaw)
//              PitchToPitch (harmony — key coupling with OCEANDEEP)
//
//  Key coupling: OPENSKY x OCEANDEEP = "The Full Column"
//      (entire mythology in one patch — sky feeds pitch down, deep feeds
//       rumble up, together they span the full frequency and emotional range)
//
//  DOCTRINES: D001-D006 compliant
//      D001: Velocity shapes filter brightness + shimmer intensity
//      D002: 2 LFOs, mod wheel, aftertouch, 4 macros
//      D003: N/A (not physically modeled)
//      D004: All 50 parameters wired to DSP
//      D005: Breathing LFO with rate floor <= 0.01 Hz
//      D006: Velocity->timbre, aftertouch->shimmer, mod wheel->filter
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  I. PRIMITIVES — Low-level DSP building blocks
//
//==============================================================================

//==============================================================================
// SkyNoiseGen — xorshift32 PRNG for shimmer diffusion and chorus modulation.
//==============================================================================
class SkyNoiseGen
{
public:
    void seed(uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(static_cast<int32_t>(state)) / 2147483648.0f;
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// SkyADSR — uses the shared StandardADSR implementation.
//==============================================================================
using SkyADSR = StandardADSR;

//==============================================================================
// SkyBreathingLFO — D005: Autonomous breathing modulation.
// Rate floor <= 0.01 Hz for sub-audible evolution.
//==============================================================================
struct SkyBreathingLFO
{
    float phase = 0.0f;
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    void prepare(double sampleRate) noexcept { sr = static_cast<float>(sampleRate); }
    void reset() noexcept { phase = 0.0f; }

    float process(float rateHz) noexcept
    {
        float out = fastSin(phase * 6.28318530718f);
        phase += rateHz / sr;
        if (phase >= 1.0f)
            phase -= 1.0f;
        return out;
    }
};

//==============================================================================
// SkyLFO — uses the shared StandardLFO implementation.
//==============================================================================
using SkyLFO = StandardLFO;

//==============================================================================
//
//  II. SUPERSAW OSCILLATOR — 7 detuned saws with spread control
//
//  Band-limited via PolyBLEP. The central saw is at the fundamental, 3 saws
//  are detuned above and 3 below. Detune spread is exponential for a
//  perceptually even spread across the frequency range.
//
//==============================================================================
struct SkySupersaw
{
    static constexpr int kNumSaws = 7;

    std::array<PolyBLEP, kNumSaws> saws;
    float baseFreq = 440.0f;
    float spread = 0.3f; // 0 = mono, 1 = max detune (~60 cents per voice)

    void reset() noexcept
    {
        for (auto& s : saws)
            s.reset();
    }

    void setFrequency(float freq, float sampleRate) noexcept
    {
        baseFreq = freq;
        // Detune offsets in semitones: center + 3 above + 3 below
        // Max spread: ±0.5 semitones (60 cents) at spread=1.0
        static constexpr float kDetuneOffsets[kNumSaws] = {0.0f, -0.12f, 0.12f, -0.24f, 0.24f, -0.42f, 0.42f};

        for (int i = 0; i < kNumSaws; ++i)
        {
            float detuneSemitones = kDetuneOffsets[i] * spread;
            float detuneRatio = fastPow2(detuneSemitones / 12.0f);
            saws[i].setFrequency(freq * detuneRatio, sampleRate);
            saws[i].setWaveform(PolyBLEP::Waveform::Saw);
        }
    }

    float processSample() noexcept
    {
        float sum = 0.0f;
        for (auto& s : saws)
            sum += s.processSample();
        // Normalize by number of saws, with slight gain to compensate
        return sum * (1.0f / static_cast<float>(kNumSaws)) * 1.4f;
    }
};

//==============================================================================
//
//  III. SHIMMER REVERB — Pitch-shifted reverb tail
//
//  A feedback delay network where the feedback path includes pitch shifting
//  via allpass interpolation. Two pitch intervals (octave up, fifth up) are
//  mixed into the reverb tail, creating the signature shimmering quality.
//
//  The shimmer feeds back into the reverb, so it builds on itself over time.
//  GLOW macro controls this feedback — more glow = longer, more harmonic tail.
//
//==============================================================================
class SkyShimmerReverb
{
public:
    // Safety cap: 2 seconds at 192kHz. Runtime maxDelay_ is derived from sampleRate in prepare().
    static constexpr int kAbsoluteMaxDelay = 384001;
    static constexpr int kNumCombs = 4;
    static constexpr int kNumAllpass = 2;

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);

        // SR-scaled grain size: 2048 samples at 44.1kHz ≈ 46ms grain duration.
        // Clamped so two half-size grains always fit within kShimmerBufSize.
        grainSize_ = std::min(static_cast<float>(kShimmerBufSize / 2 - 1),
                              std::max(512.0f, 2048.0f * static_cast<float>(sampleRate) / 44100.0f));

        // Runtime buffer size: 1 second worth of samples at the actual sample rate,
        // clamped to the absolute safety cap. Allocated here (never in renderBlock).
        maxDelay_ = std::min(kAbsoluteMaxDelay, static_cast<int>(sampleRate) + 1);

        for (auto& buf : combBuf)
            buf.assign(maxDelay_, 0.0f);
        for (auto& buf : apBuf)
            buf.assign(maxDelay_, 0.0f);

        // Prime-number delay lengths for dense, non-repeating reverb
        int combLens[kNumCombs] = {1117, 1277, 1399, 1523};
        int apLens[kNumAllpass] = {241, 557};

        // Scale by sample rate ratio
        float srRatio = sampleRate / 44100.0;
        for (int i = 0; i < kNumCombs; ++i)
        {
            combLength[i] = std::min(maxDelay_ - 1, static_cast<int>(combLens[i] * srRatio));
            combPos[i] = 0;
            combDamp[i] = 0.0f;
        }
        for (int i = 0; i < kNumAllpass; ++i)
        {
            apLength[i] = std::min(maxDelay_ - 1, static_cast<int>(apLens[i] * srRatio));
            apPos[i] = 0;
        }

        // Pitch shifter delay lines (for octave-up and fifth-up shimmer)
        shimmerPhaseOct = 0.0f;
        shimmerPhaseFifth = 0.0f;
        std::fill(shimmerBuf.begin(), shimmerBuf.end(), 0.0f);
        shimmerWritePos = 0;
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            std::fill(combBuf[i].begin(), combBuf[i].end(), 0.0f);
            combPos[i] = 0;
            combDamp[i] = 0.0f;
        }
        for (int i = 0; i < kNumAllpass; ++i)
        {
            std::fill(apBuf[i].begin(), apBuf[i].end(), 0.0f);
            apPos[i] = 0;
        }
        std::fill(shimmerBuf.begin(), shimmerBuf.end(), 0.0f);
        shimmerWritePos = 0;
        shimmerPhaseOct = 0.0f;
        shimmerPhaseFifth = 0.0f;
    }

    // Process a single sample through the shimmer reverb.
    // shimmerMix: 0-1, how much pitch-shifted signal feeds back
    // size: 0-1, controls comb filter feedback (decay time)
    // damping: 0-1, high-frequency damping in reverb tail
    // shimmerFeedback: 0-1, how much shimmer feeds back
    void processSample(float inputL, float inputR, float& outL, float& outR, float size, float damping,
                       float shimmerMix, float shimmerFeedback, float octBal = 0.6f) noexcept
    {
        float input = (inputL + inputR) * 0.5f;

        // --- Pitch Shifting (shimmer) ---
        // Write input + feedback into shimmer buffer
        shimmerBuf[shimmerWritePos] = flushDenormal(input + lastShimmerOut * shimmerFeedback);
        shimmerWritePos = (shimmerWritePos + 1) % kShimmerBufSize;

        // Octave up: read at 2x speed using two crossfading grains
        float shimmerOct = readShimmerGrain(shimmerPhaseOct, 2.0f);
        // Fifth up: read at 1.5x speed
        float shimmerFifth = readShimmerGrain(shimmerPhaseFifth, 1.5f);

        float shimmerOut = (shimmerOct * octBal + shimmerFifth * (1.0f - octBal)) * shimmerMix;
        lastShimmerOut = flushDenormal(shimmerOut);

        // Feed shimmer into the reverb network
        float reverbInput = input + shimmerOut;

        // --- Comb filters (parallel) ---
        float combOut = 0.0f;
        float feedback = 0.3f + size * 0.65f; // [0.3, 0.95]
        feedback = std::min(feedback, 0.95f);

        for (int i = 0; i < kNumCombs; ++i)
        {
            float rd = combBuf[i][combPos[i]];
            // Low-pass damping in the feedback path
            combDamp[i] = flushDenormal(rd * (1.0f - damping) + combDamp[i] * damping);
            combBuf[i][combPos[i]] = flushDenormal(reverbInput + combDamp[i] * feedback);
            combPos[i] = (combPos[i] + 1) % combLength[i];
            combOut += rd;
        }
        combOut *= 0.25f;

        // --- Allpass filters (series) ---
        float apOut = combOut;
        for (int i = 0; i < kNumAllpass; ++i)
        {
            float rd = apBuf[i][apPos[i]];
            float w = apOut + rd * 0.5f;
            apBuf[i][apPos[i]] = flushDenormal(w);
            apOut = rd - apOut * 0.5f;
            apPos[i] = (apPos[i] + 1) % apLength[i];
        }

        apOut = flushDenormal(apOut);

        // Stereo decorrelation: slight delay offset between L and R
        outL = apOut;
        outR = apOut * 0.95f + combOut * 0.05f; // tiny difference for width
    }

private:
    static constexpr int kShimmerBufSize = 8192;

    // Read from shimmer buffer at a given pitch ratio using crossfading grains
    float readShimmerGrain(float& grainPhase, float pitchRatio) noexcept
    {
        // Grain-based pitch shifting with two overlapping grains.
        // NOTE: grain size scales with sr to keep grain duration constant (~46ms at 44.1kHz).
        // Using sr-scaled grainSize computed in prepare() avoids SR-dependent artifact frequency.
        const float kGrainSize = grainSize_;

        // Advance the grain phase at (pitchRatio - 1) speed
        grainPhase += (pitchRatio - 1.0f);
        if (grainPhase >= kGrainSize)
            grainPhase -= kGrainSize;
        if (grainPhase < 0.0f)
            grainPhase += kGrainSize;

        // Two grains offset by half the grain size
        float pos1 = grainPhase;
        float pos2 = grainPhase + kGrainSize * 0.5f;
        if (pos2 >= kGrainSize)
            pos2 -= kGrainSize;

        // Hann window for crossfade
        float fade1 = 0.5f - 0.5f * fastCos(pos1 / kGrainSize * 6.28318530718f);
        float fade2 = 0.5f - 0.5f * fastCos(pos2 / kGrainSize * 6.28318530718f);

        // Read from shimmer buffer with linear interpolation.
        // idx = floor(pos) samples back; idxPrev = floor(pos)+1 samples back (older).
        // frac=0 → exactly idx; frac=1 → exactly idxPrev. Correct delay-line fractional read.
        auto readBuf = [&](float pos) -> float
        {
            int ipos = static_cast<int>(pos);
            int idx     = (shimmerWritePos - ipos     + kShimmerBufSize) % kShimmerBufSize;
            int idxPrev = (shimmerWritePos - ipos - 1 + kShimmerBufSize) % kShimmerBufSize;
            float frac = pos - static_cast<float>(ipos);
            return shimmerBuf[idx] * (1.0f - frac) + shimmerBuf[idxPrev] * frac;
        };

        return readBuf(pos1) * fade1 + readBuf(pos2) * fade2;
    }

    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    // SR-scaled grain size — maintains ~46ms grain duration independent of sample rate.
    // Clamped to [512, kShimmerBufSize/2] to keep two non-overlapping grains in the buffer.
    float grainSize_ = 2048.0f;

    // Comb filter state — vectors sized to maxDelay_ in prepare(), never in renderBlock
    std::vector<float> combBuf[kNumCombs];
    int combLength[kNumCombs]{};
    int combPos[kNumCombs]{};
    float combDamp[kNumCombs]{};

    // Allpass filter state — vectors sized to maxDelay_ in prepare(), never in renderBlock
    std::vector<float> apBuf[kNumAllpass];
    int apLength[kNumAllpass]{};
    int apPos[kNumAllpass]{};

    int maxDelay_ = 48000; // updated in prepare() — do not use before prepare() is called

    // Shimmer pitch shifter
    std::array<float, kShimmerBufSize> shimmerBuf{};
    int shimmerWritePos = 0;
    float shimmerPhaseOct = 0.0f;
    float shimmerPhaseFifth = 0.0f;
    float lastShimmerOut = 0.0f;
};

//==============================================================================
//
//  IV. STEREO CHORUS — 3-voice chorus with slow LFO modulation
//
//  Three delay taps with slowly modulated delay times create width and
//  movement. Each voice has its own LFO phase offset (120 degrees apart)
//  for a lush, even spread.
//
//==============================================================================
class SkyChorus
{
public:
    static constexpr int kMaxDelay = 4096;
    static constexpr int kNumVoices = 3;

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        for (auto& buf : delayBuf)
            std::fill(buf.begin(), buf.end(), 0.0f);
        writePos = 0;
        for (int i = 0; i < kNumVoices; ++i)
            lfoPhase[i] = static_cast<float>(i) / static_cast<float>(kNumVoices);
    }

    void reset() noexcept
    {
        for (auto& buf : delayBuf)
            std::fill(buf.begin(), buf.end(), 0.0f);
        writePos = 0;
    }

    // Process stereo in-place
    // rate: LFO rate in Hz
    // depth: modulation depth (0-1)
    // mix: wet/dry mix (0-1)
    void processSample(float& left, float& right, float rate, float depth, float mix) noexcept
    {
        // Write input to delay buffer
        delayBuf[0][writePos] = left;
        delayBuf[1][writePos] = right;

        float wetL = 0.0f, wetR = 0.0f;

        for (int v = 0; v < kNumVoices; ++v)
        {
            // LFO modulates delay time
            float lfo = fastSin(lfoPhase[v] * 6.28318530718f);
            lfoPhase[v] += rate / sr;
            if (lfoPhase[v] >= 1.0f)
                lfoPhase[v] -= 1.0f;

            // Center delay ~7ms, modulated by depth (±3ms at full depth)
            float delaySamples = (7.0f + lfo * 3.0f * depth) * sr * 0.001f;
            delaySamples = clamp(delaySamples, 1.0f, static_cast<float>(kMaxDelay - 2));

            // Read with linear interpolation
            int delayInt = static_cast<int>(delaySamples);
            float frac = delaySamples - static_cast<float>(delayInt);

            int readPos = (writePos - delayInt + kMaxDelay) % kMaxDelay;
            int readPosNext = (readPos - 1 + kMaxDelay) % kMaxDelay;

            float sampleL = delayBuf[0][readPos] * (1.0f - frac) + delayBuf[0][readPosNext] * frac;
            float sampleR = delayBuf[1][readPos] * (1.0f - frac) + delayBuf[1][readPosNext] * frac;

            // Pan each chorus voice across the stereo field
            static constexpr float kPan[kNumVoices] = {-0.7f, 0.0f, 0.7f};
            float panL = 1.0f - std::max(0.0f, kPan[v]);
            float panR = 1.0f + std::min(0.0f, kPan[v]);

            wetL += sampleL * panL;
            wetR += sampleR * panR;
        }

        wetL *= (1.0f / static_cast<float>(kNumVoices));
        wetR *= (1.0f / static_cast<float>(kNumVoices));

        left = left * (1.0f - mix) + wetL * mix;
        right = right * (1.0f - mix) + wetR * mix;

        writePos = (writePos + 1) % kMaxDelay;
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    std::array<std::array<float, kMaxDelay>, 2> delayBuf{}; // L, R
    int writePos = 0;
    float lfoPhase[kNumVoices]{};
};

//==============================================================================
//
//  V. SKY VOICE — Per-voice state for the OpenSky engine
//
//  Each voice contains:
//    - A supersaw oscillator stack (7 saws)
//    - SVF filters (LP + HP for tonal shaping)
//    - ADSR envelope
//    - Per-voice pitch envelope for RISE macro
//    - Unison sub-voices handled at the engine level
//
//==============================================================================
struct SkyVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // Supersaw stack per unison voice
    static constexpr int kMaxUnison = 7;
    std::array<SkySupersaw, kMaxUnison> unisonSaws;
    int activeUnison = 1;
    float unisonNormGain = 1.0f; // cached = 1/sqrt(activeUnison) — set at noteOn, avoids per-sample sqrt

    // Separate L and R filter instances — running both channels through the same
    // CytomicSVF state sequentially contaminates the integrators (bug found 2026-04-20).
    CytomicSVF lpfL;
    CytomicSVF lpfR;
    CytomicSVF hpfL;
    CytomicSVF hpfR;

    // Pitch envelope (for RISE macro)
    float pitchEnvLevel = 0.0f;
    float pitchEnvRate = 0.0f; // decay rate per sample

    // Amp envelope
    SkyADSR ampEnv;

    // Filter envelope (D001: velocity -> filter brightness)
    float filterEnvLevel = 0.0f;

    // Voice stealing crossfade
    float fadeOutLevel = 0.0f;

    // Cached per-unison frequencies from the previous sample — avoids calling
    // setFrequency (which iterates all 7 internal saws) when pitch hasn't changed.
    std::array<float, kMaxUnison> lastUniFreq{};
    float lastSawSpread = -1.0f; // sentinel: force update on first render
};

//==============================================================================
//
//  VI. OPENSKY ENGINE — The complete SynthEngine implementation
//
//==============================================================================
class OpenSkyEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    //-- SynthEngine interface -------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        shimmerL.prepare(sr);
        // shimmerR is intentionally not used — shimmerL processes stereo in-place
        chorus.prepare(sr);
        breathLfo.prepare(sr);
        lfo1.setRate(0.08f, srf); // initialize phase increment before first block
        lfo2.setRate(2.0f, srf);
        lfo1.reset();
        lfo2.reset();

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.fadeOutLevel = 0.0f;
            voice.ampEnv.reset();
            for (auto& saw : voice.unisonSaws)
                saw.reset();
            voice.lpfL.reset(); voice.lpfL.setMode(CytomicSVF::Mode::LowPass);
            voice.lpfR.reset(); voice.lpfR.setMode(CytomicSVF::Mode::LowPass);
            voice.hpfL.reset(); voice.hpfL.setMode(CytomicSVF::Mode::HighPass);
            voice.hpfR.reset(); voice.hpfR.setMode(CytomicSVF::Mode::HighPass);
        }

        // SRO SilenceGate: shimmer reverb tails need a generous hold — 500ms
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.ampEnv.reset();
            voice.fadeOutLevel = 0.0f;
            voice.pitchEnvLevel = 0.0f;
            voice.filterEnvLevel = 0.0f;
            voice.lastSawSpread = -1.0f;
            voice.lastUniFreq.fill(0.0f);
            for (auto& saw : voice.unisonSaws)
                saw.reset();
            voice.lpfL.reset(); voice.lpfR.reset();
            voice.hpfL.reset(); voice.hpfR.reset();
        }
        shimmerL.reset();
        chorus.reset();
        breathLfo.reset();
        lfo1.reset();
        lfo2.reset();
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        externalFMMod = 0.0f;
        modWheelAmount_ = 0.0f;
        aftertouch_ = 0.0f;
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;
        jassert(sr > 0.0f);
        if (sr <= 0.0f) { buffer.clear(); return; }

        // SRO SilenceGate: wake on note-on, bypass when silent
        for (const auto& md : midi)
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        // --- ParamSnapshot: read all parameters once per block ---
        const float sawSpread = pLoad(pSawSpread, 0.3f);
        const float sawMix = pLoad(pSawMix, 0.8f);
        const float subLevel = pLoad(pSubLevel, 0.2f);
        const int subWaveIdx = static_cast<int>(pLoad(pSubWave, 0.0f));

        const float filterCutoff = pLoad(pFilterCutoff, 6000.0f);
        const float filterReso = pLoad(pFilterReso, 0.2f);
        const float filterHPCutoff = pLoad(pFilterHP, 80.0f);
        const float filterEnvAmt = pLoad(pFilterEnvAmount, 0.4f);
        const int filterType = static_cast<int>(pLoad(pFilterType, 0.0f));

        const float shimmerMix = pLoad(pShimmerMix, 0.3f);
        const float shimmerSize = pLoad(pShimmerSize, 0.6f);
        const float shimmerDamp = pLoad(pShimmerDamping, 0.3f);
        const float shimmerFB = pLoad(pShimmerFeedback, 0.4f);
        const float shimmerOctBal = pLoad(pShimmerOctave, 0.6f);

        const float chorusRate = pLoad(pChorusRate, 0.5f);
        const float chorusDepth = pLoad(pChorusDepth, 0.4f);
        const float chorusMix = pLoad(pChorusMix, 0.3f);

        const int unisonCount =
            std::max(1, std::min(SkyVoice::kMaxUnison, static_cast<int>(pLoad(pUnisonCount, 1.0f))));
        const float unisonDetune = pLoad(pUnisonDetune, 0.2f);
        const float unisonSpread = pLoad(pUnisonSpread, 0.5f);

        const float pitchEnvAmt = pLoad(pPitchEnvAmount, 0.0f);
        const float pitchEnvDecay = pLoad(pPitchEnvDecay, 0.3f);
        const float coarseTune = pLoad(pCoarseTune, 0.0f);
        const float fineTune = pLoad(pFineTune, 0.0f);

        const float attack = pLoad(pAttack, 0.01f);
        const float decay = pLoad(pDecay, 0.3f);
        const float sustain = pLoad(pSustain, 0.7f);
        const float release = pLoad(pRelease, 0.5f);
        const float level = pLoad(pLevel, 0.8f);
        const float pan = pLoad(pPan, 0.5f);

        const float lfo1Rate = pLoad(pLfo1Rate, 0.08f);
        const float lfo1Depth = pLoad(pLfo1Depth, 0.15f);
        const int lfo1ShapeIdx = static_cast<int>(pLoad(pLfo1Shape, 0.0f));
        const float lfo2Rate = pLoad(pLfo2Rate, 2.0f);
        const float lfo2Depth = pLoad(pLfo2Depth, 0.0f);
        const int lfo2ShapeIdx = static_cast<int>(pLoad(pLfo2Shape, 0.0f));

        const float stereoWidth = pLoad(pStereoWidth, 0.5f);

        // D002: Mod matrix — read slot values (routing applied below after LFOs are computed)
        const int   modSlot1Src = static_cast<int>(pLoad(pModSlot1Src, 0.0f));
        const int   modSlot1Dst = static_cast<int>(pLoad(pModSlot1Dst, 0.0f));
        const float modSlot1Amt = pLoad(pModSlot1Amt, 0.0f);
        const int   modSlot2Src = static_cast<int>(pLoad(pModSlot2Src, 0.0f));
        const int   modSlot2Dst = static_cast<int>(pLoad(pModSlot2Dst, 0.0f));
        const float modSlot2Amt = pLoad(pModSlot2Amt, 0.0f);

        // D002: Macro reads
        const float macroRise = pLoad(pMacroRise, 0.5f);
        const float macroWidth = pLoad(pMacroWidth, 0.5f);
        const float macroGlow = pLoad(pMacroGlow, 0.5f);
        const float macroAir = pLoad(pMacroAir, 0.5f);

        // --- Macro modulation (bipolar from center 0.5) ---
        // RISE: pitch envelope up + filter opens + shimmer increases
        float riseOffset = (macroRise - 0.5f) * 2.0f;
        float effectivePitchEnvAmt = clamp(pitchEnvAmt + riseOffset * 12.0f, -24.0f, 24.0f);
        float effectiveFilterCutoff = clamp(filterCutoff + riseOffset * 6000.0f, 20.0f, 20000.0f);
        float effectiveShimmerMix = clamp(shimmerMix + riseOffset * 0.3f, 0.0f, 1.0f);

        // WIDTH: chorus depth + unison spread
        float widthOffset = (macroWidth - 0.5f) * 2.0f;
        float effectiveChorusDepth = clamp(chorusDepth + widthOffset * 0.4f, 0.0f, 1.0f);
        float effectiveChorusMix = clamp(chorusMix + widthOffset * 0.3f, 0.0f, 1.0f);
        float effectiveUnisonSpread = clamp(unisonSpread + widthOffset * 0.4f, 0.0f, 1.0f);

        // GLOW: shimmer tail length + feedback
        float glowOffset = (macroGlow - 0.5f) * 2.0f;
        float effectiveShimmerSize = clamp(shimmerSize + glowOffset * 0.3f, 0.0f, 1.0f);
        float effectiveShimmerFB = clamp(shimmerFB + glowOffset * 0.3f, 0.0f, 0.85f);

        // AIR: reverb size + high-frequency content
        float airOffset = (macroAir - 0.5f) * 2.0f;
        effectiveShimmerSize = clamp(effectiveShimmerSize + airOffset * 0.2f, 0.0f, 1.0f);
        effectiveFilterCutoff = clamp(effectiveFilterCutoff + airOffset * 4000.0f, 20.0f, 20000.0f);

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), effectivePitchEnvAmt, pitchEnvDecay, unisonCount);
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                // Kill all voices but preserve coupling state, pitch bend, and
                // output cache — full reset() would glitch coupling consumers.
                for (auto& v : voices)
                {
                    v.active = false;
                    v.ampEnv.kill();
                    v.fadeOutLevel = 0.0f;
                }
                shimmerL.reset();
                chorus.reset();
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount_ = msg.getControllerValue() / 127.0f; // D006: mod wheel -> filter
            else if (msg.isChannelPressure())
                aftertouch_ = msg.getChannelPressureValue() / 127.0f; // D006: aftertouch -> shimmer
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // D006: Mod wheel opens filter, aftertouch increases shimmer
        effectiveFilterCutoff = clamp(effectiveFilterCutoff + modWheelAmount_ * 6000.0f, 20.0f, 20000.0f);
        effectiveShimmerMix = clamp(effectiveShimmerMix + aftertouch_ * 0.4f, 0.0f, 1.0f);

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;
        float fmMod = externalFMMod;
        externalFMMod = 0.0f;

        // Apply coupling to filter
        effectiveFilterCutoff = clamp(effectiveFilterCutoff + filterMod, 20.0f, 20000.0f);

        // --- Set up LFOs ---
        lfo1.setRate(lfo1Rate, srf);
        lfo1.setShape(lfo1ShapeIdx);
        lfo2.setRate(lfo2Rate, srf);
        lfo2.setShape(lfo2ShapeIdx);

        // D005: breathing LFO modulates filter cutoff
        float breathLfoVal = breathLfo.process(lfo1Rate) * lfo1Depth;
        effectiveFilterCutoff = clamp(effectiveFilterCutoff + breathLfoVal * 2000.0f, 20.0f, 20000.0f);

        // D002: Mod matrix — block-rate routing for both slots.
        // Sources: 0=off, 1=LFO1, 2=LFO2, 3=Breath, 4=ModWheel, 5=Aftertouch, 6=Velocity(avg)
        // Destinations: 0=off, 1=FilterCutoff, 2=ShimmerMix, 3=ChorusDepth, 4=PitchEnv, 5=Level
        // Amounts are bipolar [-1, +1].
        auto applyModSlot = [&](int src, int dst, float amt) {
            if (src == 0 || dst == 0 || amt == 0.0f) return;
            float modVal = 0.0f;
            switch (src)
            {
            case 1: modVal = breathLfoVal;              break; // LFO1 (breath tracks lfo1)
            case 2: modVal = 0.0f;                      break; // LFO2 — per-sample, skip block-rate
            case 3: modVal = breathLfoVal;              break; // Breath
            case 4: modVal = modWheelAmount_;           break; // ModWheel [0,1] → bipolar via amt sign
            case 5: modVal = aftertouch_;               break; // Aftertouch
            default: break;
            }
            switch (dst)
            {
            case 1: effectiveFilterCutoff  = clamp(effectiveFilterCutoff  + modVal * amt * 8000.0f, 20.0f, 20000.0f); break;
            case 2: effectiveShimmerMix    = clamp(effectiveShimmerMix    + modVal * amt * 0.5f,    0.0f, 1.0f);       break;
            case 3: effectiveChorusDepth   = clamp(effectiveChorusDepth   + modVal * amt * 0.5f,    0.0f, 1.0f);       break;
            case 4: effectivePitchEnvAmt   = clamp(effectivePitchEnvAmt   + modVal * amt * 12.0f, -24.0f, 24.0f);      break;
            default: break;
            }
        };
        applyModSlot(modSlot1Src, modSlot1Dst, modSlot1Amt);
        applyModSlot(modSlot2Src, modSlot2Dst, modSlot2Amt);
        // D002: Mod matrix — 2 slots. Src: 0=LFO1, 1=LFO2, 2=ModWheel, 3=Aftertouch.
        // Dst: 0=FilterCutoff, 1=PitchOffset (semitones), 2=ShimmerMix, 3=Amp.
        // LFO snapshot: calling process() once here for a block-rate mod sample is intentional;
        // the per-sample loop calls process() again each sample for audio-rate LFO output.
        // The one-sample offset is inaudible at block-rate mod routing.
        float modSrcValues[4] = {
            lfo1.process() * lfo1Depth,  // 0: LFO1 (block-rate snapshot)
            lfo2.process() * lfo2Depth,  // 1: LFO2 (block-rate snapshot)
            modWheelAmount_,             // 2: ModWheel [0,1]
            aftertouch_                  // 3: Aftertouch [0,1]
        };
        // Slot 1
        float modMatrixAmpMod = 0.0f;
        {
            const int src1   = static_cast<int>(clamp(pLoad(pModSlot1Src, 0.0f), 0.0f, 3.0f));
            const int dst1   = static_cast<int>(clamp(pLoad(pModSlot1Dst, 0.0f), 0.0f, 3.0f));
            const float amt1 = pLoad(pModSlot1Amt, 0.0f);
            const float mod1 = modSrcValues[src1] * amt1;
            if (dst1 == 0)      effectiveFilterCutoff = clamp(effectiveFilterCutoff + mod1 * 8000.0f, 20.0f, 20000.0f);
            else if (dst1 == 1) pitchMod              = pitchMod + mod1 * 12.0f; // ±12 semitones at full amt
            else if (dst1 == 2) effectiveShimmerMix   = clamp(effectiveShimmerMix + mod1 * 0.5f, 0.0f, 1.0f);
            else if (dst1 == 3) modMatrixAmpMod       += mod1;  // applied to gain in per-sample loop
        }
        // Slot 2
        {
            const int src2   = static_cast<int>(clamp(pLoad(pModSlot2Src, 0.0f), 0.0f, 3.0f));
            const int dst2   = static_cast<int>(clamp(pLoad(pModSlot2Dst, 0.0f), 0.0f, 3.0f));
            const float amt2 = pLoad(pModSlot2Amt, 0.0f);
            const float mod2 = modSrcValues[src2] * amt2;
            if (dst2 == 0)      effectiveFilterCutoff = clamp(effectiveFilterCutoff + mod2 * 8000.0f, 20.0f, 20000.0f);
            else if (dst2 == 1) pitchMod              = pitchMod + mod2 * 12.0f;
            else if (dst2 == 2) effectiveShimmerMix   = clamp(effectiveShimmerMix + mod2 * 0.5f, 0.0f, 1.0f);
            else if (dst2 == 3) modMatrixAmpMod       += mod2;  // accumulated from both slots
        }

        // Set up envelopes for all active voices
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.ampEnv.setParams(attack, decay, sustain, release, srf);

            // Filter mode (applied to both L and R instances)
            CytomicSVF::Mode lpMode = (filterType == 1) ? CytomicSVF::Mode::BandPass : CytomicSVF::Mode::LowPass;
            voice.lpfL.setMode(lpMode);
            voice.lpfR.setMode(lpMode);
            voice.hpfL.setMode(CytomicSVF::Mode::HighPass);
            voice.hpfR.setMode(CytomicSVF::Mode::HighPass);

            // #620: HPF cutoff is constant within the block — set coefficients
            // once here rather than per-sample to avoid redundant trig inside the loop.
            voice.hpfL.setCoefficients_fast(filterHPCutoff, 0.0f, srf);
            voice.hpfR.setCoefficients_fast(filterHPCutoff, 0.0f, srf);
        }

        // Filter-envelope decay coefficient — block-rate constant avoids std::max in per-sample loop
        const float filterEnvDecayCoeff = 1.0f / (std::max(0.01f, decay) * srf);

        float peakEnv = 0.0f;

        // --- Render audio per sample ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            // Advance LFOs
            float lfo1Val = lfo1.process() * lfo1Depth;
            float lfo2Val = lfo2.process() * lfo2Depth;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // --- Amp envelope ---
                float envVal = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // Voice stealing crossfade
                float stealFade = 1.0f;
                if (voice.fadeOutLevel > 0.0f)
                {
                    voice.fadeOutLevel -= 1.0f / (0.005f * srf);
                    if (voice.fadeOutLevel <= 0.0f)
                        voice.fadeOutLevel = 0.0f;
                    stealFade = 1.0f - voice.fadeOutLevel;
                }

                // --- Pitch ---
                // D001: Velocity scales filter envelope (filterEnvAmt alone — velocity
                // is applied once at velCutoffMod below, not here, to avoid squaring it)
                float velFilterEnv = filterEnvAmt;

                // Pitch envelope decays toward zero (RISE effect)
                voice.pitchEnvLevel *= (1.0f - voice.pitchEnvRate);
                voice.pitchEnvLevel = flushDenormal(voice.pitchEnvLevel);

                // Filter envelope decays (rate hoisted to block-rate constant)
                voice.filterEnvLevel *= (1.0f - filterEnvDecayCoeff);
                voice.filterEnvLevel = flushDenormal(voice.filterEnvLevel);

                float midiNote = static_cast<float>(voice.noteNumber) + coarseTune + fineTune * 0.01f +
                                 voice.pitchEnvLevel + pitchMod + lfo1Val * 2.0f // LFO1 -> pitch (±2 semitones)
                                 + lfo2Val * 0.5f                                // LFO2 -> subtle pitch vibrato
                                 + pitchBendNorm * 2.0f;

                // FM coupling modulation
                float fmModPitch = fmMod * 4.0f; // ±4 semitones at full FM
                midiNote += fmModPitch;

                float baseFreq = 440.0f * fastPow2((midiNote - 69.0f) / 12.0f);
                if (baseFreq < 1.0f)
                    baseFreq = 1.0f;
                if (baseFreq > 20000.0f)
                    baseFreq = 20000.0f;

                // --- Render unison voices ---
                float voiceL = 0.0f, voiceR = 0.0f;
                int numUnison = voice.activeUnison;

                for (int u = 0; u < numUnison; ++u)
                {
                    // Unison detune: spread voices symmetrically
                    float unisonOffset = 0.0f;
                    if (numUnison > 1)
                    {
                        float position = static_cast<float>(u) / static_cast<float>(numUnison - 1) - 0.5f;
                        unisonOffset = position * unisonDetune; // detune in semitones
                    }

                    float uniFreq = baseFreq * fastPow2(unisonOffset / 12.0f);
                    // Only call setFrequency when pitch or spread actually changed —
                    // avoids iterating all 7 internal saws on every sample (perf).
                    if (uniFreq != voice.lastUniFreq[u] || sawSpread != voice.lastSawSpread)
                    {
                        voice.unisonSaws[u].spread = sawSpread;
                        voice.unisonSaws[u].setFrequency(uniFreq, srf);
                        voice.lastUniFreq[u] = uniFreq;
                    }

                    float sawOut = voice.unisonSaws[u].processSample() * sawMix;

                    // Sub oscillator (only on first unison voice) — D004: subWaveIdx dispatches
                    // 0=Sine, 1=Triangle, 2=Square. All three shapes use the same phase source
                    // (half-rate phase of the center saw) for phase-coherent sub tracking.
                    float subOut = 0.0f;
                    if (u == 0 && subLevel > 0.001f)
                    {
                        const float subPhase = voice.unisonSaws[0].saws[0].getPhase() * 0.5f;
                        switch (subWaveIdx)
                        {
                        default:
                        case 0:
                            subOut = fastSin(subPhase * 6.28318530718f);
                            break;
                        case 1:
                            subOut = 4.0f * std::fabs(subPhase - 0.5f) - 1.0f;
                            break; // triangle
                        case 2:
                            subOut = (subPhase < 0.5f) ? 1.0f : -1.0f;
                            break; // square
                        }
                        subOut *= subLevel;
                    }

                    float voiceSample = sawOut + subOut;

                    // Stereo placement per unison voice
                    float unisonPan = 0.5f;
                    if (numUnison > 1)
                    {
                        float pos = static_cast<float>(u) / static_cast<float>(numUnison - 1);
                        unisonPan = lerp(0.5f - effectiveUnisonSpread * 0.5f, 0.5f + effectiveUnisonSpread * 0.5f, pos);
                    }

                    voiceL += voiceSample * (1.0f - unisonPan);
                    voiceR += voiceSample * unisonPan;
                }

                // Update cached spread after iterating all unison voices
                voice.lastSawSpread = sawSpread;

                // Normalize by unison count
                float unisonNorm = 1.0f / std::sqrt(static_cast<float>(std::max(1, numUnison)));
                voiceL *= unisonNorm;
                voiceR *= unisonNorm;

                // --- Filter ---
                // D001: velocity drives filter brightness
                float velCutoffMod = voice.velocity * velFilterEnv * 8000.0f;
                float filterEnvMod = voice.filterEnvLevel * filterEnvAmt * 6000.0f;
                float voiceCutoff = clamp(effectiveFilterCutoff + velCutoffMod + filterEnvMod, 20.0f, 20000.0f);

                voice.lpfL.setCoefficients_fast(voiceCutoff, filterReso, srf);
                voice.lpfR.setCoefficients_fast(voiceCutoff, filterReso, srf);
                // Normalize by unison count (cached at noteOn — was per-sample std::sqrt).
                voiceL *= voice.unisonNormGain;
                voiceR *= voice.unisonNormGain;

                // --- Filter ---
                // D001: velocity drives filter brightness (coeff refresh decimated to every 16 samples)
                // HPF coefficients are set once per block (block-rate setup above)

                // Apply filters (series: HP -> LP). Separate L/R instances maintain
                // independent integrator state to avoid inter-channel contamination.
                float filteredL = voice.hpfL.processSample(voiceL);
                filteredL = voice.lpfL.processSample(filteredL);
                float filteredR = voice.hpfR.processSample(voiceR);
                filteredR = voice.lpfR.processSample(filteredR);

                // Apply envelope and velocity (+ mod matrix amp destination)
                float gain = envVal * voice.velocity * stealFade * clamp(1.0f + modMatrixAmpMod, 0.0f, 2.0f);
                mixL += filteredL * gain;
                mixR += filteredR * gain;

                peakEnv = std::max(peakEnv, envVal);
            }

            // --- Post-voice processing (shared across all voices) ---

            // Apply master level
            mixL *= level;
            mixR *= level;

            // --- Shimmer Reverb ---
            if (effectiveShimmerMix > 0.001f || effectiveShimmerSize > 0.001f)
            {
                float shimOutL = 0.0f, shimOutR = 0.0f;
                shimmerL.processSample(mixL, mixR, shimOutL, shimOutR, effectiveShimmerSize, shimmerDamp,
                                       effectiveShimmerMix, effectiveShimmerFB, shimmerOctBal);

                // Blend shimmer with dry signal
                float wetDry = effectiveShimmerMix;
                mixL = mixL * (1.0f - wetDry * 0.5f) + shimOutL * wetDry;
                mixR = mixR * (1.0f - wetDry * 0.5f) + shimOutR * wetDry;
            }

            // --- Stereo Chorus ---
            if (effectiveChorusMix > 0.001f)
            {
                chorus.processSample(mixL, mixR, chorusRate, effectiveChorusDepth, effectiveChorusMix);
            }

            // --- Stereo Width ---
            {
                float mid = (mixL + mixR) * 0.5f;
                float side = (mixL - mixR) * 0.5f;
                float widthFactor = stereoWidth * 2.0f; // 0 = mono, 0.5 = normal, 1.0 = wide
                mixL = mid + side * widthFactor;
                mixR = mid - side * widthFactor;
            }

            // --- Pan ---
            {
                float panL = std::min(1.0f, 2.0f * (1.0f - pan));
                float panR = std::min(1.0f, 2.0f * pan);
                mixL *= panL;
                mixR *= panR;
            }

            // Soft clip output to prevent digital overs
            mixL = softClip(mixL);
            mixR = softClip(mixR);

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sample, mixL);
                buffer.addSample(1, sample, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sample, (mixL + mixR) * 0.5f);
            }

            // Cache output for coupling reads
            if (sample < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(sample)] = mixL;
                outputCacheR[static_cast<size_t>(sample)] = mixR;
            }
        }

        envelopeOutput = peakEnv;

        // SRO SilenceGate: feed output to the gate for silence detection
        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size())
            return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size())
            return outputCacheR[si];
        if (channel == 2)
            return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            externalFilterMod += amount * 6000.0f;
            break;

        case CouplingType::LFOToPitch:
        case CouplingType::AmpToPitch:
        case CouplingType::PitchToPitch:
            externalPitchMod += amount * 0.5f;
            break;

        case CouplingType::AudioToFM:
            externalFMMod += amount * 0.3f;
            break;

        default:
            break;
        }
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // === OSCILLATOR ===

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_sawSpread", 1}, "Sky Saw Spread",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_sawMix", 1}, "Sky Saw Mix",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                     0.8f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_subLevel", 1}, "Sky Sub Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"sky_subWave", 1}, "Sky Sub Wave",
            // Order matches the switch: 0=Sine, 1=Triangle, 2=Square (was "Sine,Square,Triangle" — wrong)
            juce::StringArray{"Sine", "Triangle", "Square"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_coarseTune", 1}, "Sky Coarse Tune",
                                                        juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_fineTune", 1}, "Sky Fine Tune",
                                                        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

        // === PITCH ENVELOPE ===

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_pitchEnvAmount", 1}, "Sky Pitch Env Amount",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_pitchEnvDecay", 1}, "Sky Pitch Env Decay",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

        // === FILTER ===

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_filterCutoff", 1}, "Sky Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 6000.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_filterReso", 1}, "Sky Filter Reso",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_filterHP", 1}, "Sky HP Cutoff",
            juce::NormalisableRange<float>(20.0f, 2000.0f, 0.1f, 0.3f), 80.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_filterEnvAmount", 1}, "Sky Filter Env Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"sky_filterType", 1}, "Sky Filter Type", juce::StringArray{"LowPass", "BandPass"}, 0));

        // === SHIMMER REVERB ===

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_shimmerMix", 1}, "Sky Shimmer Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_shimmerSize", 1}, "Sky Shimmer Size",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_shimmerDamping", 1}, "Sky Shimmer Damping",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_shimmerFeedback", 1}, "Sky Shimmer Feedback",
            juce::NormalisableRange<float>(0.0f, 0.85f, 0.01f), 0.4f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_shimmerOctave", 1}, "Sky Shimmer Octave Balance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.6f));

        // === CHORUS ===

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_chorusRate", 1}, "Sky Chorus Rate",
            juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.4f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_chorusDepth", 1}, "Sky Chorus Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_chorusMix", 1}, "Sky Chorus Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        // === UNISON ===

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_unisonCount", 1}, "Sky Unison Count",
                                                        juce::NormalisableRange<float>(1.0f, 7.0f, 1.0f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_unisonDetune", 1}, "Sky Unison Detune",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_unisonSpread", 1}, "Sky Unison Spread",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // === AMP ENVELOPE ===

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_attack", 1}, "Sky Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_decay", 1}, "Sky Decay", juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f),
            0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_sustain", 1}, "Sky Sustain",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                     0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_release", 1}, "Sky Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_level", 1}, "Sky Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_pan", 1}, "Sky Pan", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // === LFOs ===

        // D005: LFO1 — breathing LFO with rate floor <= 0.01 Hz
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_lfo1Rate", 1}, "Sky LFO1 Rate",
            juce::NormalisableRange<float>(0.005f, 20.0f, 0.001f, 0.3f), 0.08f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_lfo1Depth", 1}, "Sky LFO1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"sky_lfo1Shape", 1}, "Sky LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // LFO2 — faster modulation source
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sky_lfo2Rate", 1}, "Sky LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 50.0f, 0.01f, 0.3f), 2.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_lfo2Depth", 1}, "Sky LFO2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"sky_lfo2Shape", 1}, "Sky LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // === STEREO ===

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_stereoWidth", 1}, "Sky Stereo Width",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // === MACROS (D002) ===

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_macroRise", 1}, "RISE",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_macroWidth", 1}, "WIDTH",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_macroGlow", 1}, "GLOW",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_macroAir", 1}, "AIR",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // === MOD MATRIX DESTINATIONS (D002) ===

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_modSlot1Src", 1}, "Sky Mod 1 Source",
                                                        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_modSlot1Dst", 1}, "Sky Mod 1 Dest",
                                                        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_modSlot1Amt", 1}, "Sky Mod 1 Amount",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_modSlot2Src", 1}, "Sky Mod 2 Source",
                                                        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_modSlot2Dst", 1}, "Sky Mod 2 Dest",
                                                        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sky_modSlot2Amt", 1}, "Sky Mod 2 Amount",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        // Total: 50 parameters
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pSawSpread = apvts.getRawParameterValue("sky_sawSpread");
        pSawMix = apvts.getRawParameterValue("sky_sawMix");
        pSubLevel = apvts.getRawParameterValue("sky_subLevel");
        pSubWave = apvts.getRawParameterValue("sky_subWave");
        pCoarseTune = apvts.getRawParameterValue("sky_coarseTune");
        pFineTune = apvts.getRawParameterValue("sky_fineTune");
        pPitchEnvAmount = apvts.getRawParameterValue("sky_pitchEnvAmount");
        pPitchEnvDecay = apvts.getRawParameterValue("sky_pitchEnvDecay");
        pFilterCutoff = apvts.getRawParameterValue("sky_filterCutoff");
        pFilterReso = apvts.getRawParameterValue("sky_filterReso");
        pFilterHP = apvts.getRawParameterValue("sky_filterHP");
        pFilterEnvAmount = apvts.getRawParameterValue("sky_filterEnvAmount");
        pFilterType = apvts.getRawParameterValue("sky_filterType");
        pShimmerMix = apvts.getRawParameterValue("sky_shimmerMix");
        pShimmerSize = apvts.getRawParameterValue("sky_shimmerSize");
        pShimmerDamping = apvts.getRawParameterValue("sky_shimmerDamping");
        pShimmerFeedback = apvts.getRawParameterValue("sky_shimmerFeedback");
        pShimmerOctave = apvts.getRawParameterValue("sky_shimmerOctave");
        pChorusRate = apvts.getRawParameterValue("sky_chorusRate");
        pChorusDepth = apvts.getRawParameterValue("sky_chorusDepth");
        pChorusMix = apvts.getRawParameterValue("sky_chorusMix");
        pUnisonCount = apvts.getRawParameterValue("sky_unisonCount");
        pUnisonDetune = apvts.getRawParameterValue("sky_unisonDetune");
        pUnisonSpread = apvts.getRawParameterValue("sky_unisonSpread");
        pAttack = apvts.getRawParameterValue("sky_attack");
        pDecay = apvts.getRawParameterValue("sky_decay");
        pSustain = apvts.getRawParameterValue("sky_sustain");
        pRelease = apvts.getRawParameterValue("sky_release");
        pLevel = apvts.getRawParameterValue("sky_level");
        pPan = apvts.getRawParameterValue("sky_pan");
        pLfo1Rate = apvts.getRawParameterValue("sky_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("sky_lfo1Depth");
        pLfo1Shape = apvts.getRawParameterValue("sky_lfo1Shape");
        pLfo2Rate = apvts.getRawParameterValue("sky_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("sky_lfo2Depth");
        pLfo2Shape = apvts.getRawParameterValue("sky_lfo2Shape");
        pStereoWidth = apvts.getRawParameterValue("sky_stereoWidth");
        pMacroRise = apvts.getRawParameterValue("sky_macroRise");
        pMacroWidth = apvts.getRawParameterValue("sky_macroWidth");
        pMacroGlow = apvts.getRawParameterValue("sky_macroGlow");
        pMacroAir = apvts.getRawParameterValue("sky_macroAir");
        pModSlot1Src = apvts.getRawParameterValue("sky_modSlot1Src");
        pModSlot1Dst = apvts.getRawParameterValue("sky_modSlot1Dst");
        pModSlot1Amt = apvts.getRawParameterValue("sky_modSlot1Amt");
        pModSlot2Src = apvts.getRawParameterValue("sky_modSlot2Src");
        pModSlot2Dst = apvts.getRawParameterValue("sky_modSlot2Dst");
        pModSlot2Amt = apvts.getRawParameterValue("sky_modSlot2Amt");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "OpenSky"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFFF8C00); } // Sunburst
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        return count;
    }

private:
    //--------------------------------------------------------------------------
    // Helper: safe parameter load with default
    static float pLoad(std::atomic<float>* p, float def) noexcept { return (p != nullptr) ? p->load() : def; }

    //--------------------------------------------------------------------------
    void noteOn(int noteNumber, float velocity, float pitchEnvAmount, float pitchEnvDecay, int unisonCount)
    {
        int idx = findFreeVoice();
        auto& voice = voices[static_cast<size_t>(idx)];

        // Smooth fade-out if stealing
        if (voice.active)
            voice.fadeOutLevel = voice.ampEnv.level;
        else
            voice.fadeOutLevel = 0.0f;

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.activeUnison = unisonCount;
        voice.unisonNormGain = 1.0f / std::sqrt(static_cast<float>(std::max(1, unisonCount)));

        // Reset amp envelope
        voice.ampEnv.reset();
        voice.ampEnv.noteOn();

        // D001: Pitch envelope — velocity scales the initial pitch offset
        voice.pitchEnvLevel = pitchEnvAmount * velocity;
        voice.pitchEnvRate = (pitchEnvDecay > 0.001f) ? (1.0f / (pitchEnvDecay * srf)) : 1.0f;

        // D001: Filter envelope — velocity triggers brightness burst
        voice.filterEnvLevel = velocity;

        // Reset frequency cache so setFrequency is forced on first render
        voice.lastSawSpread = -1.0f;
        voice.lastUniFreq.fill(0.0f);

        // Reset only active unison voices — idle slots need no work
        for (int u = 0; u < unisonCount; ++u)
        {
            voice.unisonSaws[u].reset();
            // Randomize initial phases for rich supersaw texture
            for (auto& saw : voice.unisonSaws[u].saws)
            {
                // Simple hash for pseudo-random starting phase
                uint32_t hash =
                    static_cast<uint32_t>(noteNumber * 7 + u * 31 + (&saw - &voice.unisonSaws[u].saws[0]) * 13);
                hash = hash * 2654435761u;
                saw.setPhase(static_cast<float>(hash & 0xFFFF) / 65536.0f);
            }
        }

        // Reset filters (both L and R instances)
        voice.lpfL.reset(); voice.lpfL.setMode(CytomicSVF::Mode::LowPass);
        voice.lpfR.reset(); voice.lpfR.setMode(CytomicSVF::Mode::LowPass);
        voice.hpfL.reset(); voice.hpfL.setMode(CytomicSVF::Mode::HighPass);
        voice.hpfR.reset(); voice.hpfR.setMode(CytomicSVF::Mode::HighPass);
    }

    void noteOff(int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && voice.ampEnv.stage != StandardADSR::Stage::Release)
            {
                voice.ampEnv.noteOff();
            }
        }
    }

    int findFreeVoice()
    {
        return VoiceAllocator::findFreeVoicePreferRelease(voices, kMaxVoices, [](const SkyVoice& v)
                                                          { return v.ampEnv.stage == StandardADSR::Stage::Release; });
    }

    //--------------------------------------------------------------------------
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use
    std::array<SkyVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;

    // Shared FX (post-voice, applied to summed output)
    // shimmerR removed: SkyShimmerReverb.processSample() already emits stereo L+R in a
    // single call. The second instance was allocated (38 KB) but never referenced in
    // renderBlock — dead weight discovered in DSP review 2026-04-20.
    SkyShimmerReverb shimmerL;
    SkyChorus chorus;

    // D005: breathing LFO
    SkyBreathingLFO breathLfo;

    // D002: Modulation LFOs
    SkyLFO lfo1;
    SkyLFO lfo2;

    // MIDI expression (D006)
    float modWheelAmount_ = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range
    float aftertouch_ = 0.0f;

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;
    float externalFMMod = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers (50 params)
    std::atomic<float>* pSawSpread = nullptr;
    std::atomic<float>* pSawMix = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pSubWave = nullptr;
    std::atomic<float>* pCoarseTune = nullptr;
    std::atomic<float>* pFineTune = nullptr;
    std::atomic<float>* pPitchEnvAmount = nullptr;
    std::atomic<float>* pPitchEnvDecay = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pFilterHP = nullptr;
    std::atomic<float>* pFilterEnvAmount = nullptr;
    std::atomic<float>* pFilterType = nullptr;
    std::atomic<float>* pShimmerMix = nullptr;
    std::atomic<float>* pShimmerSize = nullptr;
    std::atomic<float>* pShimmerDamping = nullptr;
    std::atomic<float>* pShimmerFeedback = nullptr;
    std::atomic<float>* pShimmerOctave = nullptr;
    std::atomic<float>* pChorusRate = nullptr;
    std::atomic<float>* pChorusDepth = nullptr;
    std::atomic<float>* pChorusMix = nullptr;
    std::atomic<float>* pUnisonCount = nullptr;
    std::atomic<float>* pUnisonDetune = nullptr;
    std::atomic<float>* pUnisonSpread = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pPan = nullptr;
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;
    std::atomic<float>* pStereoWidth = nullptr;
    std::atomic<float>* pMacroRise = nullptr;
    std::atomic<float>* pMacroWidth = nullptr;
    std::atomic<float>* pMacroGlow = nullptr;
    std::atomic<float>* pMacroAir = nullptr;
    std::atomic<float>* pModSlot1Src = nullptr;
    std::atomic<float>* pModSlot1Dst = nullptr;
    std::atomic<float>* pModSlot1Amt = nullptr;
    std::atomic<float>* pModSlot2Src = nullptr;
    std::atomic<float>* pModSlot2Dst = nullptr;
    std::atomic<float>* pModSlot2Amt = nullptr;
};

} // namespace xoceanus
