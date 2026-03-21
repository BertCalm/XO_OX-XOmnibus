#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
// DubNoiseGen — xorshift32 PRNG for noise oscillator and flutter modulation.
//==============================================================================
class DubNoiseGen
{
public:
    void seed (uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// DubAnalogDrift — LP-filtered noise simulating vintage oscillator instability.
// Returns pitch offset in cents (±5 max at full amount).
//==============================================================================
class DubAnalogDrift
{
public:
    void prepare (double sampleRate) noexcept
    {
        constexpr double twoPi = 6.28318530717958647692;
        lpCoeff = static_cast<float> (1.0 - std::exp (-twoPi * 0.5 / sampleRate));
    }

    void reset() noexcept { smoothed = 0.0f; }

    void seed (uint32_t s) noexcept { noise.seed (s); }

    float process (float amount) noexcept
    {
        float raw = noise.process();
        smoothed += lpCoeff * (raw - smoothed);
        return smoothed * amount * 5.0f;
    }

private:
    DubNoiseGen noise;
    float smoothed = 0.0f;
    float lpCoeff = 0.001f;
};

//==============================================================================
// DubPitchEnvelope — Exponential pitch envelope for log drum sounds.
// Returns pitch offset in semitones, decaying from depth to 0.
//==============================================================================
class DubPitchEnvelope
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; }
    void reset() noexcept { level = 0.0f; }

    void trigger() noexcept { level = 1.0f; }

    void setParams (float depthSemitones, float decayTimeSec) noexcept
    {
        depth = depthSemitones;
        float clamped = std::max (decayTimeSec, 0.001f);
        decayCoeff = static_cast<float> (std::exp (-1.0 / (static_cast<double> (clamped) * sr)));
    }

    float process() noexcept
    {
        float offset = level * depth;
        level *= decayCoeff;
        if (level < 0.0001f) level = 0.0f;
        return offset;
    }

private:
    double sr = 44100.0;
    float level = 0.0f;
    float depth = 0.0f;
    float decayCoeff = 0.99f;
};

// DubAdsrEnvelope replaced by StandardADSR (Source/DSP/StandardADSR.h).
// All call sites updated to use xomnibus::StandardADSR directly.

// DubLFO replaced by StandardLFO (Source/DSP/StandardLFO.h).
// StandardLFO has identical waveforms (Sine=0, Triangle=1, Saw=2, Square=3, S&H=4)
// and the same Knuth TAOCP LCG for S&H. setRate() now takes (hz, sampleRate).
// prepare(sr) calls removed — StandardLFO has no prepare(); SR is passed to setRate().

//==============================================================================
// DubTapeDelay — Tape delay with wow & flutter, bandpass feedback, Hermite interp.
//
// The tape character comes from: feedback bandpass narrowing ("wear"),
// wow (0.3 Hz sine) and flutter (~45 Hz smoothed noise) modulating delay time,
// and tanh saturation in the feedback path.
//==============================================================================
class DubTapeDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        int maxSamples = static_cast<int> (sr * 2.0) + 4;
        bufferL.resize (static_cast<size_t> (maxSamples), 0.0f);
        bufferR.resize (static_cast<size_t> (maxSamples), 0.0f);
        bufferSize = maxSamples;
        writePos = 0;

        fbFilterL.reset();
        fbFilterL.setMode (CytomicSVF::Mode::BandPass);
        fbFilterR.reset();
        fbFilterR.setMode (CytomicSVF::Mode::BandPass);

        wowPhase = 0.0;
        flutterNoise.seed (77777);
        flutterSmoothed = 0.0f;
        constexpr float twoPi = 6.28318530717958647692f;
        flutterCoeff = 1.0f - std::exp (-twoPi * 45.0f / static_cast<float> (sampleRate));
    }

    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
        fbFilterL.reset();
        fbFilterR.reset();
        wowPhase = 0.0;
        flutterSmoothed = 0.0f;
    }

    void processStereo (float* left, float* right, int numSamples,
                        float delayTimeSec, float feedback, float wear,
                        float wowAmount, float mix) noexcept
    {
        if (bufferSize <= 0) return;

        float srf = static_cast<float> (sr);
        float bpCenter = 200.0f + (4000.0f - 200.0f - wear * 2300.0f) * 0.5f + wear * 150.0f;
        float bpQ = 0.3f + wear * 0.4f;
        fbFilterL.setCoefficients (bpCenter, bpQ, srf);
        fbFilterR.setCoefficients (bpCenter, bpQ, srf);

        float delaySamples = static_cast<float> (delayTimeSec * sr);

        // wow LFO: accumulate phase in radians to avoid per-sample * twoPi multiply.
        constexpr double twoPi = 6.28318530717958647692;
        const double wowPhaseInc = 0.3 * twoPi / sr;  // radians per sample, block-constant

        for (int i = 0; i < numSamples; ++i)
        {
            float wowMod = fastSin (static_cast<float> (wowPhase)) * wowAmount * 0.002f;
            wowPhase += wowPhaseInc;
            if (wowPhase >= twoPi) wowPhase -= twoPi;

            float flutterRaw = flutterNoise.process();
            flutterSmoothed += flutterCoeff * (flutterRaw - flutterSmoothed);
            float flutter = flutterSmoothed * wowAmount * 0.0005f;

            float modulatedDelay = delaySamples * (1.0f + wowMod + flutter);
            modulatedDelay = std::max (1.0f, std::min (modulatedDelay, static_cast<float> (bufferSize - 2)));

            float readPosF = static_cast<float> (writePos) - modulatedDelay;
            if (readPosF < 0.0f) readPosF += static_cast<float> (bufferSize);

            // Offset R by 5 samples (≈0.1ms Haas spread) — below fusion threshold
            // so it sounds wider rather than as a distinct echo.
            float readPosFR = readPosF - 5.0f;
            if (readPosFR < 0.0f) readPosFR += static_cast<float> (bufferSize);

            float delayedL = cubicInterp (bufferL.data(), bufferSize, readPosF);
            float delayedR = cubicInterp (bufferR.data(), bufferSize, readPosFR);

            // Feedback path: bandpass filter + tanh saturation
            float fbL = fbFilterL.processSample (delayedL);
            float fbR = fbFilterR.processSample (delayedR);
            fbL = fastTanh (fbL);
            fbR = fastTanh (fbR);

            bufferL[static_cast<size_t> (writePos)] = flushDenormal (left[i] + fbL * feedback);
            bufferR[static_cast<size_t> (writePos)] = flushDenormal (right[i] + fbR * feedback);

            left[i]  = left[i]  * (1.0f - mix) + delayedL * mix;
            right[i] = right[i] * (1.0f - mix) + delayedR * mix;

            writePos = (writePos + 1) % bufferSize;
        }
    }

    void clearBuffer()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
    }

private:
    static float cubicInterp (const float* buf, int size, float pos) noexcept
    {
        int i0 = static_cast<int> (pos);
        float frac = pos - static_cast<float> (i0);

        int im1 = (i0 - 1 + size) % size;
        int i1  = (i0 + 1) % size;
        int i2  = (i0 + 2) % size;
        i0 = i0 % size;

        float y0 = buf[im1], y1 = buf[i0], y2 = buf[i1], y3 = buf[i2];

        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    double sr = 44100.0;
    std::vector<float> bufferL, bufferR;
    int bufferSize = 0;
    int writePos = 0;

    CytomicSVF fbFilterL, fbFilterR;

    double wowPhase = 0.0;
    DubNoiseGen flutterNoise;
    float flutterSmoothed = 0.0f;
    float flutterCoeff = 0.001f;
};

//==============================================================================
// DubSpringReverb — 6-stage allpass chain with true stereo decorrelation.
//
// Tuned for metallic spring tank character. Left and right channels use
// offset allpass buffer lengths for natural stereo width. One-pole damping
// LP post-chain softens the high end.
//==============================================================================
class DubSpringReverb
{
public:
    static constexpr int kNumAllpass = 6;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        double ratio = sampleRate / 44100.0;

        const int baseLengthsL[] = { 142, 107, 379, 277, 211, 163 };
        const int baseLengthsR[] = { 149, 113, 389, 283, 223, 173 };

        for (int i = 0; i < kNumAllpass; ++i)
        {
            int lenL = std::max (1, static_cast<int> (static_cast<double> (baseLengthsL[i]) * ratio));
            int lenR = std::max (1, static_cast<int> (static_cast<double> (baseLengthsR[i]) * ratio));
            allpassL[i].resize (static_cast<size_t> (lenL), 0.0f);
            allpassR[i].resize (static_cast<size_t> (lenR), 0.0f);
            apLengthL[i] = lenL;
            apLengthR[i] = lenR;
            apPosL[i] = 0;
            apPosR[i] = 0;
        }

        dampL = 0.0f;
        dampR = 0.0f;
    }

    void reset()
    {
        for (int i = 0; i < kNumAllpass; ++i)
        {
            std::fill (allpassL[i].begin(), allpassL[i].end(), 0.0f);
            std::fill (allpassR[i].begin(), allpassR[i].end(), 0.0f);
            apPosL[i] = 0;
            apPosR[i] = 0;
        }
        dampL = 0.0f;
        dampR = 0.0f;
    }

    void processStereo (float* left, float* right, int numSamples,
                        float size, float damping, float mix) noexcept
    {
        float g = 0.3f + size * 0.55f;
        constexpr float twoPi = 6.28318530717958647692f;
        float dampHz = 500.0f + damping * 7500.0f;
        float dampCoeff = 1.0f - std::exp (-twoPi * dampHz / static_cast<float> (sr));

        for (int s = 0; s < numSamples; ++s)
        {
            float inL = left[s];
            float inR = right[s];

            float wetL = inL;
            float wetR = inR;

            for (int i = 0; i < kNumAllpass; ++i)
            {
                wetL = processAllpass (allpassL[i].data(), apPosL[i], wetL, g);
                wetR = processAllpass (allpassR[i].data(), apPosR[i], wetR, g * 0.98f);
            }

            dampL += dampCoeff * (wetL - dampL);
            dampR += dampCoeff * (wetR - dampR);
            wetL = dampL;
            wetR = dampR;

            if (std::abs (dampL) < 1.0e-20f) dampL = 0.0f;
            if (std::abs (dampR) < 1.0e-20f) dampR = 0.0f;

            for (int i = 0; i < kNumAllpass; ++i)
            {
                apPosL[i] = (apPosL[i] + 1) % apLengthL[i];
                apPosR[i] = (apPosR[i] + 1) % apLengthR[i];
            }

            left[s]  = inL * (1.0f - mix) + wetL * mix;
            right[s] = inR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    static float processAllpass (float* buf, int pos, float input, float g) noexcept
    {
        float delayed = buf[pos];
        float output = -g * input + delayed;
        buf[pos] = flushDenormal (input + g * output);
        return output;
    }

    double sr = 44100.0;
    std::vector<float> allpassL[kNumAllpass], allpassR[kNumAllpass];
    int apLengthL[kNumAllpass] = {};
    int apLengthR[kNumAllpass] = {};
    int apPosL[kNumAllpass] = {};
    int apPosR[kNumAllpass] = {};
    float dampL = 0.0f, dampR = 0.0f;
};

//==============================================================================
// DubVoice — per-voice state for the OVERDUB engine.
//==============================================================================
struct DubVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;   // Voice allocation timestamp for LRU stealing (VoiceAllocator)

    // Oscillators
    PolyBLEP mainOsc;
    PolyBLEP subOsc;
    DubNoiseGen noise;

    // Envelopes
    xomnibus::StandardADSR ampEnv;
    DubPitchEnvelope pitchEnv;

    // Filter
    CytomicSVF filter;

    // Drift
    DubAnalogDrift drift;

    // Glide
    float currentFreq = 261.63f;
    float glideSourceFreq = 261.63f;
    bool glideActive = false;
};

//==============================================================================
// DubEngine — Dub-focused synthesis with embedded send/return FX chain.
// (from XOverdub)
//
// Features:
//   - PolyBLEP oscillator (4 waveforms: sine, tri, saw, square/PWM)
//   - Sub oscillator (sine, -1 octave)
//   - Noise generator
//   - Cytomic SVF filter (LPF, HPF, BPF) with envelope + LFO modulation
//   - Full ADSR envelope with exponential decay/release
//   - Pitch envelope for log drum sounds
//   - Analog drift (vintage pitch instability)
//   - LFO with 3-way routing (pitch, filter, amp)
//   - Glide (portamento) with exponential slew
//   - 8-voice polyphony with poly/mono/legato modes
//   - Send/return FX chain: Drive → Tape Delay → Spring Reverb
//   - Tape delay with wow & flutter, bandpass feedback, Hermite interpolation
//   - Spring reverb (6-allpass chain, true stereo decorrelation)
//
// Coupling:
//   - Output: envelope level (for AmpToFilter, AmpToPitch on other engines)
//   - Input: AmpToFilter (external envelope modulates filter cutoff)
//           LFOToPitch (external LFO pitch modulation)
//           AmpToPitch (external amp-to-pitch coupling)
//
//==============================================================================
class DubEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface -------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        silenceGate.prepare (sampleRate, maxBlockSize);
        silenceGate.setHoldTime (500.0f); // Overdub has reverb tails
        srf = static_cast<float> (sr);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        sendBufL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        sendBufR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t> (i)];
            v.active = false;
            v.startTime = 0;
            v.mainOsc.reset();
            v.subOsc.reset();
            v.noise.seed (static_cast<uint32_t> (i * 6271 + 3));
            v.ampEnv.prepare (static_cast<float> (sr));
            v.ampEnv.reset();
            v.pitchEnv.prepare (sr);
            v.pitchEnv.reset();
            v.filter.reset();
            v.filter.setMode (CytomicSVF::Mode::LowPass);
            v.drift.prepare (sr);
            v.drift.seed (static_cast<uint32_t> (i * 7919 + 1));
        }

        // lfo.prepare() not needed — StandardLFO has no prepare(); SR passed to setRate()
        tapeDelay.prepare (sr);
        springReverb.prepare (sr);
        aftertouch.prepare (sr);  // D006: 5ms attack / 20ms release smoothing
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.mainOsc.reset();
            v.subOsc.reset();
            v.ampEnv.reset();
            v.pitchEnv.reset();
            v.filter.reset();
            v.drift.reset();
            v.glideActive = false;
        }
        lfo.reset();
        lfo2.reset();
        tapeDelay.reset();
        springReverb.reset();
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const int oscWaveIdx   = (pOscWave != nullptr) ? static_cast<int> (pOscWave->load()) : 2;
        const int oscOctaveIdx = (pOscOctave != nullptr) ? static_cast<int> (pOscOctave->load()) : 2;
        const float oscTune    = (pOscTune != nullptr) ? pOscTune->load() : 0.0f;
        const float oscPwm     = (pOscPwm != nullptr) ? pOscPwm->load() : 0.5f;
        const float subLevel   = (pSubLevel != nullptr) ? pSubLevel->load() : 0.0f;
        const float noiseLevel = (pNoiseLevel != nullptr) ? pNoiseLevel->load() : 0.0f;
        const float driftAmt   = (pDrift != nullptr) ? pDrift->load() : 0.05f;
        const float level      = (pLevel != nullptr) ? pLevel->load() : 0.8f;

        const int filterModeIdx = (pFilterMode != nullptr) ? static_cast<int> (pFilterMode->load()) : 0;
        const float filterCut  = (pFilterCutoff != nullptr) ? pFilterCutoff->load() : 8000.0f;
        const float filterRes  = (pFilterReso != nullptr) ? pFilterReso->load() : 0.0f;
        const float filterEnv  = (pFilterEnvAmt != nullptr) ? pFilterEnvAmt->load() : 0.0f;

        const float attack     = (pAttack != nullptr) ? pAttack->load() : 0.005f;
        const float decay      = (pDecay != nullptr) ? pDecay->load() : 0.3f;
        const float sustain    = (pSustain != nullptr) ? pSustain->load() : 0.7f;
        const float release    = (pRelease != nullptr) ? pRelease->load() : 0.3f;

        const float pitchDepth = (pPitchEnvDepth != nullptr) ? pPitchEnvDepth->load() : 0.0f;
        const float pitchDecay = (pPitchEnvDecay != nullptr) ? pPitchEnvDecay->load() : 0.015f;

        const float lfoRate    = (pLfoRate != nullptr) ? pLfoRate->load() : 2.0f;
        const float lfoDepth   = (pLfoDepth != nullptr) ? pLfoDepth->load() : 0.0f;
        const int lfoDest      = (pLfoDest != nullptr) ? static_cast<int> (pLfoDest->load()) : 0;

        const float sendLvl    = (pSendLevel != nullptr) ? pSendLevel->load() : 0.5f;
        const float returnLvl  = (pReturnLevel != nullptr) ? pReturnLevel->load() : 0.7f;
        const float dryLvl     = (pDryLevel != nullptr) ? pDryLevel->load() : 1.0f;

        const float driveAmt   = (pDriveAmount != nullptr) ? pDriveAmount->load() : 1.0f;
        const float delayTime  = (pDelayTime != nullptr) ? pDelayTime->load() : 0.375f;
        const float delayFb    = (pDelayFeedback != nullptr) ? pDelayFeedback->load() : 0.5f;
        const float delayWear  = (pDelayWear != nullptr) ? pDelayWear->load() : 0.3f;
        const float delayWow   = (pDelayWow != nullptr) ? pDelayWow->load() : 0.15f;
        const float delayMix   = (pDelayMix != nullptr) ? pDelayMix->load() : 0.5f;
        const float reverbSize = (pReverbSize != nullptr) ? pReverbSize->load() : 0.4f;
        const float reverbDamp = (pReverbDamp != nullptr) ? pReverbDamp->load() : 0.5f;
        const float reverbMix  = (pReverbMix != nullptr) ? pReverbMix->load() : 0.3f;

        const int voiceMode    = (pVoiceMode != nullptr) ? static_cast<int> (pVoiceMode->load()) : 0;
        const float glideAmt   = (pGlide != nullptr) ? pGlide->load() : 0.0f;
        const int maxPoly      = (pPolyphony != nullptr)
            ? (1 << std::min (3, static_cast<int> (pPolyphony->load()))) : 8;

        // Precompute glide coefficient (block-constant)
        float glideCoeff = 0.0f;
        if (glideAmt > 0.0f)
            glideCoeff = 1.0f - fastExp (-1.0f / (srf * (0.005f + glideAmt * 0.495f)));

        // Map waveform index to PolyBLEP waveform
        PolyBLEP::Waveform waveform = PolyBLEP::Waveform::Saw;
        switch (oscWaveIdx)
        {
            case 0: waveform = PolyBLEP::Waveform::Sine; break;
            case 1: waveform = PolyBLEP::Waveform::Triangle; break;
            case 2: waveform = PolyBLEP::Waveform::Saw; break;
            case 3: waveform = PolyBLEP::Waveform::Pulse; break;
        }

        // Map filter mode
        CytomicSVF::Mode filterMode = CytomicSVF::Mode::LowPass;
        switch (filterModeIdx)
        {
            case 0: filterMode = CytomicSVF::Mode::LowPass; break;
            case 1: filterMode = CytomicSVF::Mode::HighPass; break;
            case 2: filterMode = CytomicSVF::Mode::BandPass; break;
        }

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        oscOctaveIdx, oscTune, glideAmt, voiceMode, maxPoly);
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allVoicesOff();
            // D006: channel pressure → aftertouch (applied to send VCA level below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            // D006: CC#1 mod wheel → send VCA boost (+0–0.35, more signal into echo/reverb chain)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);  // channel-mode: voice 0 holds global value

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;

        // Setup LFO — main LFO rate/depth from params
        lfo.setRate (lfoRate, srf);
        bool hasLfo = lfoDepth > 0.001f;

        // DSP FIX: Secondary LFO — autonomous triangle modulation on the
        // non-targeted destination. When main LFO targets pitch, LFO2 adds
        // gentle filter movement; when main targets filter/amp, LFO2 adds
        // slow pitch drift. Rate = 1/3 of main (organic breathing rate).
        // This addresses the "weakest modulation in fleet" seance finding.
        lfo2.setRate (std::max (0.05f, lfoRate * 0.333f), srf);
        lfo2.setShape (1); // Triangle — softer movement than sine (StandardLFO::Triangle=1)
        bool hasLfo2 = lfoDepth > 0.05f; // only when main LFO is meaningfully active

        float peakEnv = 0.0f;

        // --- Render voices ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Per-sample LFO
            float lfoVal = 0.0f;
            float lfoPitchMod = 0.0f;
            float lfoCutoffMod = 0.0f;
            float lfoAmpMod = 0.0f;

            if (hasLfo)
            {
                lfoVal = lfo.process() * lfoDepth;
                switch (lfoDest)
                {
                    case 0: lfoPitchMod = lfoVal; break;
                    case 1: lfoCutoffMod = lfoVal; break;
                    case 2: lfoAmpMod = lfoVal; break;
                }
            }

            // DSP FIX: LFO2 adds complementary modulation on the non-targeted axis.
            // Depth is 30% of main — subtle but audible movement.
            if (hasLfo2)
            {
                float lfo2Val = lfo2.process() * lfoDepth * 0.3f;
                switch (lfoDest)
                {
                    case 0: lfoCutoffMod += lfo2Val; break;  // Main→Pitch, LFO2→Filter
                    case 1: lfoPitchMod  += lfo2Val * 0.5f; break; // Main→Filter, LFO2→Pitch (gentler)
                    case 2: lfoCutoffMod += lfo2Val; break;  // Main→Amp, LFO2→Filter
                }
            }

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // Update envelope params
                voice.ampEnv.setADSR (attack, decay, sustain, release);
                voice.pitchEnv.setParams (pitchDepth, pitchDecay);

                // Glide: exponential frequency slew
                float baseFreq = midiToFreqOct (voice.noteNumber, oscOctaveIdx, oscTune);

                if (voice.glideActive)
                {
                    voice.glideSourceFreq += glideCoeff * (baseFreq - voice.glideSourceFreq);

                    if (std::abs (voice.glideSourceFreq - baseFreq) < 0.1f)
                    {
                        voice.glideSourceFreq = baseFreq;
                        voice.glideActive = false;
                    }
                    baseFreq = voice.glideSourceFreq;
                }

                // Pitch modulation: pitch env + drift + LFO + coupling + pitch bend
                float pitchOffset = voice.pitchEnv.process();
                float driftCents = voice.drift.process (driftAmt);
                float totalSemitones = pitchOffset + driftCents / 100.0f
                                     + lfoPitchMod * 2.0f + pitchMod
                                     + pitchBendNorm * 2.0f;
                float freq = baseFreq * fastExp (totalSemitones * (0.693147f / 12.0f));

                // Generate oscillators
                voice.mainOsc.setFrequency (freq, srf);
                voice.mainOsc.setWaveform (waveform);
                voice.mainOsc.setPulseWidth (oscPwm);
                float oscOut = voice.mainOsc.processSample();

                voice.subOsc.setFrequency (freq * 0.5f, srf);
                voice.subOsc.setWaveform (PolyBLEP::Waveform::Sine);
                float subOut = voice.subOsc.processSample() * subLevel;

                float noiseOut = voice.noise.process() * noiseLevel;
                float raw = oscOut + subOut + noiseOut;

                // Filter with envelope + LFO + coupling modulation
                float envVal = voice.ampEnv.process();
                float cutoffMod = filterCut;

                if (std::abs (filterEnv) > 0.001f)
                {
                    float envOffset = filterEnv * envVal * voice.velocity * 10000.0f;
                    cutoffMod = std::max (20.0f, std::min (20000.0f, cutoffMod + envOffset));
                }
                if (std::abs (lfoCutoffMod) > 0.001f)
                {
                    cutoffMod *= fastExp (lfoCutoffMod * 2.0f * 0.693147f);
                    cutoffMod = std::max (20.0f, std::min (20000.0f, cutoffMod));
                }
                // External coupling filter modulation
                cutoffMod += filterMod * 2000.0f;
                cutoffMod = std::max (20.0f, std::min (20000.0f, cutoffMod));

                voice.filter.setMode (filterMode);
                voice.filter.setCoefficients_fast (cutoffMod, filterRes, srf);
                float filtered = voice.filter.processSample (raw);

                // Apply envelope and velocity
                float out = filtered * envVal * voice.velocity;

                // LFO amp modulation
                if (std::abs (lfoAmpMod) > 0.001f)
                {
                    float ampMod = 1.0f + lfoAmpMod * 0.5f;
                    out *= std::max (0.0f, ampMod);
                }

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                }

                // Mono voice sum → stereo
                mixL += out;
                mixR += out;

                peakEnv = std::max (peakEnv, envVal);
            }

            // --- Send/Return FX chain ---
            // D006: aftertouch boosts send VCA level (more dub send on pressure).
            // Sensitivity 0.3: full pressure adds up to +30% more signal into
            // the tape delay / spring reverb send chain — classic dub technique of
            // pressing a key harder to push more signal into the echo returns.
            // D006: mod wheel also boosts send VCA (+0–0.35) — performance send control.
            const float effectiveSendLvl = std::min (1.0f, sendLvl + atPressure * 0.3f + modWheelAmount * 0.35f);
            // Send bus: voice output × effective send level
            sendBufL[static_cast<size_t> (sample)] = mixL * effectiveSendLvl;
            sendBufR[static_cast<size_t> (sample)] = mixR * effectiveSendLvl;

            // Dry path
            float dryL = mixL * dryLvl;
            float dryR = mixR * dryLvl;

            // Store dry output for this sample (FX applied after per-sample loop)
            outputCacheL[static_cast<size_t> (sample)] = dryL;
            outputCacheR[static_cast<size_t> (sample)] = dryR;
        }

        // --- Apply FX chain to send bus (block-based) ---
        float* sendL = sendBufL.data();
        float* sendR = sendBufR.data();

        // Drive (tanh saturation on send path)
        if (driveAmt > 1.001f)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                sendL[i] = fastTanh (sendL[i] * driveAmt);
                sendR[i] = fastTanh (sendR[i] * driveAmt);
            }
        }

        // Tape Delay
        tapeDelay.processStereo (sendL, sendR, numSamples,
                                delayTime, delayFb, delayWear, delayWow, delayMix);

        // Spring Reverb
        springReverb.processStereo (sendL, sendR, numSamples,
                                   reverbSize, reverbDamp, reverbMix);

        // --- Mix dry + return, write to output buffer ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float outL = outputCacheL[static_cast<size_t> (sample)]
                       + sendBufL[static_cast<size_t> (sample)] * returnLvl;
            float outR = outputCacheR[static_cast<size_t> (sample)]
                       + sendBufR[static_cast<size_t> (sample)] * returnLvl;

            // Soft limiter to prevent feedback explosion
            outL = fastTanh (outL);
            outR = fastTanh (outR);

            // Apply engine level
            outL *= level;
            outR *= level;

            // Update output cache for coupling reads
            outputCacheL[static_cast<size_t> (sample)] = outL;
            outputCacheR[static_cast<size_t> (sample)] = outR;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, outL);
                buffer.addSample (1, sample, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (outL + outR) * 0.5f);
            }
        }

        envelopeOutput = peakEnv;

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];

        // Channel 2 = envelope level (for AmpToFilter, AmpToPitch coupling)
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
            {
                // Inverted envelope drives filter (dub pump effect)
                float ducked = 1.0f - amount;
                externalFilterMod += (ducked - 0.5f) * 2.0f;
                break;
            }
            case CouplingType::AmpToPitch:
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 0.5f;
                break;

            default:
                break;
        }
    }

    //-- Parameters ------------------------------------------------------------

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
        // Oscillator
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "dub_oscWave", 1 }, "Dub Osc Wave",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "dub_oscOctave", 1 }, "Dub Octave",
            juce::StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_oscTune", 1 }, "Dub Fine Tune",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_oscPwm", 1 }, "Dub Pulse Width",
            juce::NormalisableRange<float> (0.01f, 0.99f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_subLevel", 1 }, "Dub Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_noiseLevel", 1 }, "Dub Noise Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_drift", 1 }, "Dub Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.05f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_level", 1 }, "Dub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // Filter
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "dub_filterMode", 1 }, "Dub Filter Mode",
            juce::StringArray { "LPF", "HPF", "BPF" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_filterCutoff", 1 }, "Dub Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_filterReso", 1 }, "Dub Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // D001: default raised from 0.0 to 0.25 so filter envelope is active
        // on every new patch. The existing DSP (envVal × velocity × 10000 Hz sweep)
        // is already D001-compliant; this default ensures it ships audibly active.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_filterEnvAmt", 1 }, "Dub Filter Env Amt",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.25f));

        // Amp Envelope
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_attack", 1 }, "Dub Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.3f), 0.005f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_decay", 1 }, "Dub Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_sustain", 1 }, "Dub Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_release", 1 }, "Dub Release",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

        // Pitch Envelope
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_pitchEnvDepth", 1 }, "Dub Pitch Env Depth",
            juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_pitchEnvDecay", 1 }, "Dub Pitch Env Decay",
            juce::NormalisableRange<float> (0.005f, 0.040f, 0.001f), 0.015f));

        // LFO
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_lfoRate", 1 }, "Dub LFO Rate",
            juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.3f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_lfoDepth", 1 }, "Dub LFO Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "dub_lfoDest", 1 }, "Dub LFO Dest",
            juce::StringArray { "Pitch", "Filter", "Amp" }, 0));

        // Send/Return
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_sendLevel", 1 }, "Dub Send Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_returnLevel", 1 }, "Dub Return Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_dryLevel", 1 }, "Dub Dry Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

        // Drive
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_driveAmount", 1 }, "Dub Drive",
            juce::NormalisableRange<float> (1.0f, 10.0f, 0.01f), 1.0f));

        // Tape Delay
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_delayTime", 1 }, "Dub Delay Time",
            juce::NormalisableRange<float> (0.05f, 2.0f, 0.001f, 0.3f), 0.375f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_delayFeedback", 1 }, "Dub Delay Feedback",
            juce::NormalisableRange<float> (0.0f, 1.2f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_delayWear", 1 }, "Dub Delay Wear",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_delayWow", 1 }, "Dub Delay Wow",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_delayMix", 1 }, "Dub Delay Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // Spring Reverb
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_reverbSize", 1 }, "Dub Reverb Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_reverbDamp", 1 }, "Dub Reverb Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_reverbMix", 1 }, "Dub Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // Voice
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "dub_voiceMode", 1 }, "Dub Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "dub_glide", 1 }, "Dub Glide",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "dub_polyphony", 1 }, "Dub Polyphony",
            juce::StringArray { "1", "2", "4", "8" }, 3));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscWave       = apvts.getRawParameterValue ("dub_oscWave");
        pOscOctave     = apvts.getRawParameterValue ("dub_oscOctave");
        pOscTune       = apvts.getRawParameterValue ("dub_oscTune");
        pOscPwm        = apvts.getRawParameterValue ("dub_oscPwm");
        pSubLevel      = apvts.getRawParameterValue ("dub_subLevel");
        pNoiseLevel    = apvts.getRawParameterValue ("dub_noiseLevel");
        pDrift         = apvts.getRawParameterValue ("dub_drift");
        pLevel         = apvts.getRawParameterValue ("dub_level");
        pFilterMode    = apvts.getRawParameterValue ("dub_filterMode");
        pFilterCutoff  = apvts.getRawParameterValue ("dub_filterCutoff");
        pFilterReso    = apvts.getRawParameterValue ("dub_filterReso");
        pFilterEnvAmt  = apvts.getRawParameterValue ("dub_filterEnvAmt");
        pAttack        = apvts.getRawParameterValue ("dub_attack");
        pDecay         = apvts.getRawParameterValue ("dub_decay");
        pSustain       = apvts.getRawParameterValue ("dub_sustain");
        pRelease       = apvts.getRawParameterValue ("dub_release");
        pPitchEnvDepth = apvts.getRawParameterValue ("dub_pitchEnvDepth");
        pPitchEnvDecay = apvts.getRawParameterValue ("dub_pitchEnvDecay");
        pLfoRate       = apvts.getRawParameterValue ("dub_lfoRate");
        pLfoDepth      = apvts.getRawParameterValue ("dub_lfoDepth");
        pLfoDest       = apvts.getRawParameterValue ("dub_lfoDest");
        pSendLevel     = apvts.getRawParameterValue ("dub_sendLevel");
        pReturnLevel   = apvts.getRawParameterValue ("dub_returnLevel");
        pDryLevel      = apvts.getRawParameterValue ("dub_dryLevel");
        pDriveAmount   = apvts.getRawParameterValue ("dub_driveAmount");
        pDelayTime     = apvts.getRawParameterValue ("dub_delayTime");
        pDelayFeedback = apvts.getRawParameterValue ("dub_delayFeedback");
        pDelayWear     = apvts.getRawParameterValue ("dub_delayWear");
        pDelayWow      = apvts.getRawParameterValue ("dub_delayWow");
        pDelayMix      = apvts.getRawParameterValue ("dub_delayMix");
        pReverbSize    = apvts.getRawParameterValue ("dub_reverbSize");
        pReverbDamp    = apvts.getRawParameterValue ("dub_reverbDamp");
        pReverbMix     = apvts.getRawParameterValue ("dub_reverbMix");
        pVoiceMode     = apvts.getRawParameterValue ("dub_voiceMode");
        pGlide         = apvts.getRawParameterValue ("dub_glide");
        pPolyphony     = apvts.getRawParameterValue ("dub_polyphony");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Overdub"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF6B7B3A); }
    int getMaxVoices() const override { return kMaxVoices; }

private:
    SilenceGate silenceGate;

    //--------------------------------------------------------------------------
    void noteOn (int noteNumber, float velocity,
                 int oscOctaveIdx, float oscTune, float glideAmt,
                 int voiceMode, int maxPoly)
    {
        // Mono/legato modes
        if (voiceMode == 1 || voiceMode == 2)
        {
            for (int i = 1; i < kMaxVoices; ++i)
                voices[static_cast<size_t> (i)].active = false;

            auto& v = voices[0];
            bool legatoRetrigger = !(voiceMode == 2 && v.active);

            // Glide from previous note
            if (v.active && glideAmt > 0.001f)
            {
                v.glideSourceFreq = midiToFreqOct (v.noteNumber, oscOctaveIdx, oscTune);
                v.glideActive = true;
            }
            else
            {
                v.glideActive = false;
            }

            v.active = true;
            v.noteNumber = noteNumber;
            v.velocity = velocity;
            v.startTime = voiceAllocationCounter++;

            if (legatoRetrigger)
            {
                v.ampEnv.noteOn();
                v.pitchEnv.trigger();
            }
            return;
        }

        // Poly mode: find free voice or steal oldest
        int poly = std::min (maxPoly, kMaxVoices);
        int idx = VoiceAllocator::findFreeVoice (voices, poly);
        auto& v = voices[static_cast<size_t> (idx)];

        // Glide from previous note if voice was active
        if (v.active && glideAmt > 0.001f)
        {
            v.glideSourceFreq = midiToFreqOct (v.noteNumber, oscOctaveIdx, oscTune);
            v.glideActive = true;
        }
        else
        {
            v.glideActive = false;
        }

        v.active = true;
        v.noteNumber = noteNumber;
        v.velocity = velocity;
        v.startTime = voiceAllocationCounter++;
        v.ampEnv.noteOn();
        v.pitchEnv.trigger();
        v.mainOsc.reset();
        v.subOsc.reset();
        v.filter.reset();
    }

    void noteOff (int noteNumber)
    {
        for (auto& v : voices)
            if (v.active && v.noteNumber == noteNumber)
                v.ampEnv.noteOff();
    }

    // Lightweight voice kill — no FX buffer clearing (FX tails persist, dub behavior).
    // Used for AllNotesOff/AllSoundOff to avoid expensive std::fill on audio thread.
    void allVoicesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.ampEnv.reset();
            v.pitchEnv.reset();
            v.glideActive = false;
        }
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
    }

    // findFreeVoice() replaced by VoiceAllocator::findFreeVoice() (Source/DSP/VoiceAllocator.h)

    static float midiToFreqOct (int midiNote, int octaveChoiceIndex, float tuneCents) noexcept
    {
        int octaveOffset = octaveChoiceIndex - 2;
        float n = static_cast<float> (midiNote)
                + static_cast<float> (octaveOffset) * 12.0f
                + tuneCents / 100.0f;
        return 440.0f * fastPow2 ((n - 69.0f) * (1.0f / 12.0f));
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<DubVoice, kMaxVoices> voices;
    uint64_t voiceAllocationCounter = 0;   // Monotonic counter for LRU voice stealing

    // LFO — replaced by StandardLFO (Source/DSP/StandardLFO.h)
    StandardLFO lfo;
    StandardLFO lfo2;  // DSP FIX: secondary LFO for complementary modulation

    // FX chain
    DubTapeDelay tapeDelay;
    DubSpringReverb springReverb;

    // D006: CS-80-inspired poly aftertouch (channel pressure → send VCA level)
    PolyAftertouch aftertouch;

    // ---- D006 Mod wheel — CC#1 boosts send VCA amount (+0–0.35, more echo send) ----
    float modWheelAmount = 0.0f;
    float pitchBendNorm  = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Send bus buffers (pre-allocated in prepare)
    std::vector<float> sendBufL;
    std::vector<float> sendBufR;

    // Cached APVTS parameter pointers (36 params)
    std::atomic<float>* pOscWave = nullptr;
    std::atomic<float>* pOscOctave = nullptr;
    std::atomic<float>* pOscTune = nullptr;
    std::atomic<float>* pOscPwm = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pNoiseLevel = nullptr;
    std::atomic<float>* pDrift = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pFilterMode = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pFilterEnvAmt = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pPitchEnvDepth = nullptr;
    std::atomic<float>* pPitchEnvDecay = nullptr;
    std::atomic<float>* pLfoRate = nullptr;
    std::atomic<float>* pLfoDepth = nullptr;
    std::atomic<float>* pLfoDest = nullptr;
    std::atomic<float>* pSendLevel = nullptr;
    std::atomic<float>* pReturnLevel = nullptr;
    std::atomic<float>* pDryLevel = nullptr;
    std::atomic<float>* pDriveAmount = nullptr;
    std::atomic<float>* pDelayTime = nullptr;
    std::atomic<float>* pDelayFeedback = nullptr;
    std::atomic<float>* pDelayWear = nullptr;
    std::atomic<float>* pDelayWow = nullptr;
    std::atomic<float>* pDelayMix = nullptr;
    std::atomic<float>* pReverbSize = nullptr;
    std::atomic<float>* pReverbDamp = nullptr;
    std::atomic<float>* pReverbMix = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pPolyphony = nullptr;
};

} // namespace xomnibus
