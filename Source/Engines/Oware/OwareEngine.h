// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OwareEngine.h — XOware | "The Resonant Board"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOware is the sunken oware board — a carved wooden mancala game
//      from the Akan people of Ghana, lost to the Atlantic on a trade route
//      and now encrusted with coral and bronze barnacles on the ocean floor.
//      Strike a hollow and the whole board shimmers with sympathetic resonance.
//      Seeds of metal, glass, stone, and wood fall into the cups and the
//      board remembers every impact, every vibration, every player.
//
//  ENGINE CONCEPT:
//      A tuned percussion synthesizer where the physics of struck materials
//      — wood bars, metal keys, glass bowls, bronze bells — is continuously
//      morphable. Every note is a bar on a resonating instrument; every bar
//      sympathetically excites every other sounding bar. The engine spans
//      the entire tuned percussion family from African balafon to Javanese
//      gamelan to Western vibraphone through continuous material morphing.
//
//  THE 7 PILLARS (9.8 Target):
//      1. Material Continuum — modal ratios + material exponent alpha for
//         per-mode differential decay (wood upper modes die fast, metal ring)
//      2. Mallet Physics — Chaigne contact model with spectral lowpass on
//         excitation + mallet bounce for soft strikes (D001 at physics level)
//      3. Sympathetic Resonance Network — per-mode frequency-selective
//         coupling between voices (not amplitude-based, spectrum-based)
//      4. Resonator Body — tube/frame/bowl/open with body-membrane coupling
//         via Gaussian proximity decay boost
//      5. Buzz Membrane — balafon/gyil spider-silk mirliton: BPF extraction
//         + tanh nonlinearity (band-selective, not whole-signal)
//      6. Breathing Gamelan — Balinese beat-frequency shimmer at fixed Hz
//         (not ratio-based) via shadow voice detuning
//      7. Thermal Drift — shared slow tuning drift + per-voice personality
//         seed. The feature that makes it feel alive when nobody's playing.
//
//  HISTORICAL LINEAGE:
//      West African balafon and gyil (Chaigne & Doutaut 1997), Javanese and
//      Balinese gamelan (Rossing 2000), Western marimba and vibraphone
//      (Fletcher & Rossing 1998), Tibetan singing bowls, modal synthesis
//      (Adrien 1991, Bilbao 2009). Material exponent from beam dispersion
//      theory (Fletcher & Rossing §2.3).
//
//  Accent: Akan Goldweight #B5883E (brass abrammuo — gold dust weights)
//  Parameter prefix: owr_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// Material ratio tables — from Rossing (2000) and Fletcher & Rossing (1998).
//==============================================================================
static constexpr float kWoodRatios[8] = {1.000f, 3.990f, 9.140f, 15.800f, 24.300f, 34.500f, 46.500f, 60.200f};
static constexpr float kMetalRatios[8] = {1.000f, 4.000f, 10.000f, 17.600f, 27.200f, 38.800f, 52.400f, 68.000f};
static constexpr float kBellRatios[8] = {1.000f, 1.506f, 2.000f, 2.514f, 3.011f, 3.578f, 4.170f, 4.952f};
static constexpr float kBowlRatios[8] = {1.000f, 2.710f, 5.330f, 8.860f, 13.300f, 18.640f, 24.890f, 32.040f};

//==============================================================================
// OwareMalletExciter — Chaigne contact model (1997) with spectral lowpass
// and mallet bounce for soft strikes.
//==============================================================================
struct OwareMalletExciter
{
    void trigger(float velocity, float hardness, float baseFreq, float sampleRate) noexcept
    {
        active = true;
        sampleCounter = 0;
        float contactMs = 5.0f - hardness * 4.5f;
        contactSamples = std::max(static_cast<int>(contactMs * 0.001f * sampleRate), 4);
        peakAmplitude = velocity;
        noiseMix = hardness * hardness;
        noiseState = static_cast<uint32_t>(velocity * 65535.0f) + 12345u;

        // Improvement #2: mallet contact lowpass cutoff — hard mallets excite all modes,
        // soft mallets only excite low modes (Hertz contact model)
        malletCutoff = baseFreq * (1.5f + hardness * 18.5f); // 1.5× to 20× fundamental
        malletFilterState = 0.0f;
        float fc = std::min(malletCutoff, sampleRate * 0.49f);
        malletLPCoeff = 1.0f - std::exp(-2.0f * 3.14159265f * fc / sampleRate);

        // Improvement #8: mallet bounce — soft strikes produce a secondary hit
        // at 15-25ms with 30% amplitude (the mallet bouncing off the bar)
        bounceActive = (hardness < 0.4f && velocity < 0.7f);
        bounceSample = static_cast<int>(sampleRate * (0.015f + (1.0f - hardness) * 0.010f));
        bounceAmp = peakAmplitude * 0.3f * (1.0f - hardness * 2.0f);
        if (bounceAmp < 0.0f)
            bounceAmp = 0.0f;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        float out = 0.0f;

        // Primary strike
        if (sampleCounter < contactSamples)
        {
            float phase = static_cast<float>(sampleCounter) / static_cast<float>(contactSamples);
            float pulse = fastSin(phase * 3.14159265f) * peakAmplitude;
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f) * peakAmplitude;
            out = pulse * (1.0f - noiseMix) + noise * noiseMix;
        }

        // Mallet bounce (secondary hit)
        if (bounceActive && sampleCounter >= bounceSample && sampleCounter < bounceSample + contactSamples)
        {
            int bouncePhaseIdx = sampleCounter - bounceSample;
            float phase = static_cast<float>(bouncePhaseIdx) / static_cast<float>(contactSamples);
            out += fastSin(phase * 3.14159265f) * bounceAmp;
        }

        // Shut off after both strikes complete.
        // F12: when no bounce, use contactSamples as shutdown threshold (not the
        // default bounceSample=720 sentinel which would run 720 silent samples).
        int shutdownAt = bounceActive ? bounceSample + contactSamples : contactSamples;
        if (sampleCounter >= shutdownAt)
            active = false;

        ++sampleCounter;

        // Apply mallet contact lowpass (hard = open, soft = low-passed)
        malletFilterState += malletLPCoeff * (out - malletFilterState);
        malletFilterState = flushDenormal(malletFilterState);
        return malletFilterState;
    }

    void reset() noexcept
    {
        active = false;
        sampleCounter = 0;
        malletFilterState = 0.0f;
    }

    bool active = false;
    int sampleCounter = 0, contactSamples = 48;
    float peakAmplitude = 1.0f, noiseMix = 0.0f;
    uint32_t noiseState = 12345u;
    float malletCutoff = 8000.0f, malletLPCoeff = 0.5f, malletFilterState = 0.0f;
    bool bounceActive = false;
    int bounceSample = 720;
    float bounceAmp = 0.0f;
};

//==============================================================================
// OwareMode — single 2nd-order resonator with per-mode output accessor
// for sympathetic resonance coupling (Improvement #4).
//==============================================================================
struct OwareMode
{
    void setFreqAndQ(float freqHz, float q, float sampleRate) noexcept
    {
        // W03 fix: dirty-flag cache — skip expensive exp/cos when freq and Q are
        // nearly unchanged (within 0.1% and 0.5% respectively). The shimmer LFO
        // and material smoother change slowly enough that this fires rarely.
        if (std::abs(freqHz - freq) < freq * 0.001f && std::abs(q - cachedQ) < cachedQ * 0.005f)
            return;

        if (freqHz >= sampleRate * 0.49f)
            freqHz = sampleRate * 0.49f;
        float w = 2.0f * 3.14159265f * freqHz / sampleRate;
        float bw = freqHz / std::max(q, 1.0f);
        // fastExp/fastCos/fastSin: ~0.01–0.1% error — negligible for resonator coefficients
        // (far below perceptual threshold). The dirty-flag cache above already limits
        // how often this path runs; when it does, fast-math keeps the work cheap.
        float r = fastExp(-3.14159265f * bw / sampleRate);
        a1 = 2.0f * r * fastCos(w);
        a2 = r * r;
        b0 = (1.0f - r * r) * fastSin(w);
        freq = freqHz;
        cachedQ = q;
    }

    float process(float input) noexcept
    {
        float out = b0 * input + a1 * y1 - a2 * y2;
        out = flushDenormal(out);
        y2 = y1;
        y1 = out;
        lastOutput = out; // cache for sympathetic coupling readback
        return out;
    }

    void reset() noexcept
    {
        y1 = 0.0f;
        y2 = 0.0f;
        lastOutput = 0.0f;
        freq = 0.0f;     // F11: reset to 0 so rebuildSympathyCouplingTables skips
        cachedQ = -1.0f; //      this mode until setFreqAndQ is called next render
    }

    float freq = 0.0f;     // F11: init to 0 so coupling-table builder skips unset modes
    float cachedQ = -1.0f; // W03: dirty-flag cache; -1 forces first-sample computation
    float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
    float lastOutput = 0.0f; // per-mode output for sympathetic coupling
};

//==============================================================================
// OwareBuzzMembrane — Improvement #6: BPF extraction + tanh nonlinearity.
// Extracts the 200-800 Hz band where the membrane resonates, soft-clips it,
// then re-injects. Band-selective, not whole-signal distortion.
//==============================================================================
struct OwareBuzzMembrane
{
    float process(float input, float amount, int bodyType) noexcept
    {
        if (amount < 0.001f)
            return input;

        // BPF: extract the membrane resonance band
        float buzzBand = buzzBPF.processSample(input);

        // Nonlinear: tanh on the extracted band (membrane only activates above threshold)
        float sensitivity = 5.0f + amount * 15.0f;
        float buzzed = buzzBand * (1.0f + amount * xoceanus::fastTanh(buzzBand * sensitivity));

        // Re-inject buzz artifacts
        return input + buzzed * amount;
    }

    void prepare(float sampleRate, int bodyType) noexcept
    {
        // F6: Buzz frequency matched to owr_bodyType enum (0=tube, 1=frame, 2=bowl, 3=open).
        // Prior code used bodyType==2 for "metal" but 2 is bowl in the engine's enum.
        // tube/open=300 Hz, frame=150 Hz, bowl=420 Hz (sub-octave membrane resonance).
        float buzzFreq = 300.0f;
        if (bodyType == 1)
            buzzFreq = 150.0f;
        else if (bodyType == 2)
            buzzFreq = 420.0f; // bowl: membrane resonance near sub-octave of typical pitches

        buzzBPF.setMode(CytomicSVF::Mode::BandPass);
        buzzBPF.setCoefficients(buzzFreq, 0.6f, sampleRate);
    }

    void reset() noexcept { buzzBPF.reset(); }

    CytomicSVF buzzBPF;
};

//==============================================================================
// OwareBodyResonator — Improvement #7: Gaussian proximity decay boost.
// Body modes reinforce membrane modes that are near body resonance frequencies.
//==============================================================================
struct OwareBodyResonator
{
    static constexpr int kMaxDelay = 4096;

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        frameMode1.reset();
        frameMode2.reset();
        frameMode3.reset();
    }

    void setFundamental(float freqHz) noexcept
    {
        // F9: dirty-flag — frame modes are fixed-frequency (200/580/1100 Hz) so
        //     their setFreqAndQ calls are already protected by OwareMode's dirty-flag,
        //     but the bowl trig (fastCos/fastSin) and tube delay are recomputed every
        //     sample even when freq hasn't changed (e.g., sustaining notes).
        //     Skip the whole body when freq is within 0.05% of last value.
        if (std::abs(freqHz - fundamentalHz) < fundamentalHz * 0.0005f)
            return;

        tubeDelaySamples = (freqHz > 20.0f) ? sr / freqHz : sr / 20.0f;
        tubeDelaySamples = std::min(tubeDelaySamples, static_cast<float>(kMaxDelay - 1));
        frameMode1.setFreqAndQ(200.0f, 15.0f, sr);
        frameMode2.setFreqAndQ(580.0f, 20.0f, sr);
        frameMode3.setFreqAndQ(1100.0f, 12.0f, sr);
        bowlFreq = freqHz * 0.5f;
        fundamentalHz = freqHz;
        // Cache bowl trig — recomputed only when fundamental changes (not per-sample)
        float w = 2.0f * 3.14159265f * std::max(bowlFreq, 20.0f) / sr;
        bowlCosW = xoceanus::fastCos(w);
        bowlSinW = xoceanus::fastSin(w);
    }

    // Improvement #7: compute per-mode decay boost based on proximity to body resonances
    float getModeDecayBoost(float modeFreq, int bodyType, float bodyDepth) const noexcept
    {
        if (bodyDepth < 0.001f || bodyType == 3)
            return 1.0f; // open = no boost

        float boost = 1.0f;
        float bw = 80.0f; // Gaussian bandwidth in Hz

        if (bodyType == 0) // Tube: harmonics of fundamental
        {
            for (int h = 1; h <= 4; ++h)
            {
                float bodyMode = fundamentalHz * static_cast<float>(h);
                float dist = (modeFreq - bodyMode) / bw;
                boost += bodyDepth * 0.5f * std::exp(-0.5f * dist * dist);
            }
        }
        else if (bodyType == 1) // Frame: fixed resonances
        {
            static constexpr float frameModes[3] = {200.0f, 580.0f, 1100.0f};
            for (float fm : frameModes)
            {
                float dist = (modeFreq - fm) / bw;
                boost += bodyDepth * 0.3f * std::exp(-0.5f * dist * dist);
            }
        }
        else if (bodyType == 2) // Bowl: sub-octave + fundamental
        {
            float dist1 = (modeFreq - bowlFreq) / bw;
            float dist2 = (modeFreq - fundamentalHz) / bw;
            boost += bodyDepth * 0.6f * std::exp(-0.5f * dist1 * dist1);
            boost += bodyDepth * 0.4f * std::exp(-0.5f * dist2 * dist2);
        }
        return boost;
    }

    float process(float input, int bodyType, float depth) noexcept
    {
        if (depth < 0.001f)
            return input;
        float bodyOut = 0.0f;

        switch (bodyType)
        {
        case 0:
        {
            delayLine[writePos] = input;
            float readPos = static_cast<float>(writePos) - tubeDelaySamples;
            if (readPos < 0.0f)
                readPos += kMaxDelay;
            int r0 = static_cast<int>(readPos);
            float frac = readPos - static_cast<float>(r0);
            int r1 = (r0 + 1) % kMaxDelay;
            float delayed = delayLine[r0] * (1.0f - frac) + delayLine[r1] * frac;
            bodyOut = delayed * 0.6f + input * 0.4f;
            writePos = (writePos + 1) % kMaxDelay;
            break;
        }
        case 1:
            bodyOut =
                frameMode1.process(input) * 0.5f + frameMode2.process(input) * 0.3f + frameMode3.process(input) * 0.2f;
            break;
        case 2:
        {
            // bowlCosW and bowlSinW are cached in setFundamental() — no per-sample trig
            constexpr float r = 0.999f;
            float newY1 = bowlY1 * 2.0f * r * bowlCosW - bowlY2 * r * r + input * (1.0f - r * r) * bowlSinW;
            bowlY2 = bowlY1;
            bowlY1 = flushDenormal(newY1);
            bodyOut = bowlY1;
            break;
        }
        default:
            return input;
        }
        return input * (1.0f - depth) + bodyOut * depth;
    }

    void reset() noexcept
    {
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        frameMode1.reset();
        frameMode2.reset();
        frameMode3.reset();
        bowlY1 = 0.0f;
        bowlY2 = 0.0f;
    }

    float sr = 0.0f, fundamentalHz = 440.0f;  // sr: Sentinel — set by prepare()
    std::array<float, kMaxDelay> delayLine{};
    int writePos = 0;
    float tubeDelaySamples = 100.0f;
    OwareMode frameMode1, frameMode2, frameMode3;
    float bowlFreq = 220.0f, bowlY1 = 0.0f, bowlY2 = 0.0f;
    float bowlCosW = 1.0f, bowlSinW = 0.0f; // cached per-block to avoid per-sample trig
};

//==============================================================================
// OwareVoice
//==============================================================================
struct OwareVoice
{
    static constexpr int kMaxModes = 8;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    OwareMalletExciter exciter;
    std::array<OwareMode, kMaxModes> modes;
    std::array<float, kMaxModes> modeDecayBoosts{}; // Improvement #7
    OwareBuzzMembrane buzz;
    OwareBodyResonator body;
    FilterEnvelope filterEnv;
    CytomicSVF svf;

    float ampLevel = 0.0f;
    // F2/F3: cached cutoff for SVF dirty-flag; -1 forces first-sample computation.
    // svf mode is set once at noteOn (always LowPass), not per-sample.
    float lastCutoff = -1.0f;

    // Per-voice shimmer + D002 LFOs
    StandardLFO shimmerLFO;
    StandardLFO lfo1, lfo2; // BUG-1 FIX: engine-level LFOs now backed by real objects

    // Improvement #5: per-voice thermal personality (fixed offset in cents)
    float thermalPersonality = 0.0f;

    // Cached pan gains (BUG-3 FIX: avoid per-sample trig)
    float panL = 0.707f, panR = 0.707f;

    // Sympathetic resonance sparse coupling table (CPU optimization)
    // Built at note-on, consumed per-sample. Each entry links one of this
    // voice's modes to one mode of another voice, with a precomputed gain.
    // Max 32 entries covers worst case (8 voices × ~4 coupled modes each).
    struct SympathyCoupling
    {
        int otherVoiceIdx = -1; // which voice
        int otherModeIdx = 0;   // which mode of that voice
        int thisModeIdx = 0;    // which mode of this voice receives
        float gain = 0.0f;      // precomputed proximity × scale
    };
    static constexpr int kMaxCouplings = 32;
    std::array<SympathyCoupling, kMaxCouplings> sympathyCouplings{};
    int sympathyCouplingCount = 0;

    float noteOffDecayBoost = 1.0f; // < 1.0 during release to accelerate per-sample decay

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        ampLevel = 0.0f;
        noteOffDecayBoost = 1.0f;
        lastCutoff = -1.0f;  // F2: force SVF recompute on next active sample
        sympathyCouplingCount = 0;
        glide.reset();
        exciter.reset();
        buzz.reset();
        body.reset();
        filterEnv.kill();
        shimmerLFO.reset();
        lfo1.reset();
        lfo2.reset();
        modeDecayBoosts.fill(1.0f);
        for (auto& m : modes)
            m.reset();
    }
};

//==============================================================================
// OwareEngine — "The Resonant Board" (v2: 7 Pillars + 9 Visionary Improvements)
//==============================================================================
class OwareEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Oware"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFB5883E); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].buzz.prepare(srf, 0);
            voices[i].body.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].shimmerLFO.setShape(StandardLFO::Sine);
            // Improvement #5: per-voice thermal personality from seeded PRNG
            uint32_t seed = static_cast<uint32_t>(i * 7919 + 42);
            seed = seed * 1664525u + 1013904223u;
            voices[i].thermalPersonality = (static_cast<float>(seed & 0xFFFF) / 32768.0f - 1.0f) * 2.0f;
        }

        smoothMaterial.prepare(srf);
        smoothMallet.prepare(srf);
        smoothBuzz.prepare(srf);
        smoothBodyDepth.prepare(srf);
        smoothSympathy.prepare(srf);
        smoothBrightness.prepare(srf);

        // ~0.5 second thermal drift time constant, sample-rate scaled
        thermalCoeff = 1.0f - std::exp(-1.0f / (0.5f * srf));

        // F7: precompute noteOff release coefficient — exp(log(0.3)/(0.005*sr)).
        // noteOff was computing this on every MIDI event; it's a prepare()-level constant.
        noteOffReleaseBoost = std::exp(std::log(0.3f) / (0.005f * srf));

        prepareSilenceGate(sr, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        thermalState = 0.0f;
        thermalTarget = 0.0f;
    }

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        (void)sampleIndex;
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0)
            return;
        float val = buf[numSamples - 1] * amount;
        switch (type)
        {
        case CouplingType::AmpToFilter:
            couplingFilterMod += val * 2000.0f;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += val * 2.0f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += val;
            break;
        case CouplingType::EnvToMorph:
            couplingMaterialMod += val;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Render — all 7 pillars + 9 improvements integrated
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            else if (msg.isChannelPressure())
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        if (isSilenceGateBypassed())
        {
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const float pMaterial = loadP(paramMaterial, 0.2f);
        const float pMalletBase = loadP(paramMalletHardness, 0.3f);
        const int pBodyType = static_cast<int>(loadP(paramBodyType, 0.0f));
        const float pBodyDepth = loadP(paramBodyDepth, 0.5f);
        const float pBuzzAmount = loadP(paramBuzzAmount, 0.0f);
        const float pSympathy = loadP(paramSympathyAmount, 0.3f);
        const float pShimmerHz = loadP(paramShimmerRate, 6.0f); // now in Hz (beat frequency)
        const float pBrightness = loadP(paramBrightness, 8000.0f);
        const float pDamping = loadP(paramDamping, 0.3f);
        const float pDecay = loadP(paramDecay, 2.0f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmount, 0.3f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pThermal = loadP(paramThermalDrift, 0.3f);

        const float macroMaterial = loadP(paramMacroMaterial, 0.0f);
        const float macroMallet = loadP(paramMacroMallet, 0.0f);
        const float macroCoupling = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // D006: aftertouch → mallet hardness (Moog's suggestion: continuous expression after note-on)
        float effectiveMaterial = std::clamp(pMaterial + macroMaterial * 0.8f + couplingMaterialMod, 0.0f, 1.0f);
        // D006: mod wheel → material blend (ghost feedback: material morphing as expression)
        effectiveMaterial = std::clamp(effectiveMaterial + modWheelAmount * 0.4f, 0.0f, 1.0f);
        float effectiveMallet = std::clamp(pMalletBase + macroMallet * 0.5f + aftertouchAmount * 0.4f, 0.0f, 1.0f);
        float effectiveSympathy = std::clamp(pSympathy + macroCoupling * 0.4f, 0.0f, 1.0f);
        float effectiveBodyDep = std::clamp(pBodyDepth + macroSpace * 0.3f, 0.0f, 1.0f);
        float effectiveBright = std::clamp(
            pBrightness + macroMallet * 4000.0f + aftertouchAmount * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);

        smoothMaterial.set(effectiveMaterial);
        smoothMallet.set(effectiveMallet);
        smoothBuzz.set(pBuzzAmount);
        smoothBodyDepth.set(effectiveBodyDep);
        smoothSympathy.set(effectiveSympathy);
        smoothBrightness.set(effectiveBright);

        // Snapshot pitch coupling before reset (#1118).
        const float blockCouplingPitchMod = couplingPitchMod;
        couplingFilterMod = 0.0f;
        const float capturedPitchMod = couplingPitchMod; // P25 fix: capture before zero
        couplingPitchMod = 0.0f;
        couplingMaterialMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;
        // Pitch-bend ratio: block-constant portion, hoisted above per-sample loop.
        const float blockBendRatio =
            PitchBendUtil::semitonesToFreqRatio(bendSemitones + blockCouplingPitchMod);

        // Improvement #1: material exponent alpha — controls per-mode decay differentiation
        // Wood alpha=2.0 (upper modes die fast), Metal alpha=0.3 (all modes ring together)
        float materialAlpha = 2.0f - effectiveMaterial * 1.7f; // 2.0 at wood → 0.3 at metal

        // CPU FIX: precompute log(m+1) table once per block — values are constant within a block.
        // std::log(float(m+1)) was called 8x per voice per sample inside the inner loop.
        float logModeIndex[OwareVoice::kMaxModes];
        for (int m = 0; m < OwareVoice::kMaxModes; ++m)
            logModeIndex[m] = std::log(static_cast<float>(m + 1));

        // Improvement #1 follow-up: precompute fastExp(-materialAlpha * logModeIndex[m])
        // once per block. Both inputs are block-constant (materialAlpha from pMaterial,
        // logModeIndex is precomputed above), so the per-mode per-voice per-sample
        // fastExp call inside the inner loop collapses to an array lookup. Was
        // kMaxModes × numVoices × numSamples fastExp calls per block; now kMaxModes.
        float modeDecayScale[OwareVoice::kMaxModes];
        for (int m = 0; m < OwareVoice::kMaxModes; ++m)
            modeDecayScale[m] = fastExp(-materialAlpha * logModeIndex[m]);

        float decayTimeSec = std::max(pDecay * (1.0f - pDamping * 0.8f), 0.01f);
        float baseDecayCoeff = fastExp(-1.0f / (decayTimeSec * srf));

        // Improvement #5: thermal drift — shared slow tuning scalar.
        // F5: thermalTimer must count samples, not blocks, to honour the "~4 seconds"
        // comment. Block-count increments make the timer run blockSize× too slow
        // (e.g., 128 blocks at 96kHz = ~0.000133s per tick, not 1 sample = 10.4 µs).
        thermalTimer += numSamples;
        if (thermalTimer > static_cast<int>(srf * 4.0f)) // new target every ~4 seconds
        {
            thermalNoiseState = thermalNoiseState * 1664525u + 1013904223u;
            thermalTarget =
                (static_cast<float>(thermalNoiseState & 0xFFFF) / 32768.0f - 1.0f) * pThermal * 8.0f; // max ±8 cents
            thermalTimer = 0;
        }
        // thermalCoeff is sample-rate-scaled (~0.5s time constant, computed in prepare()).
        // Replaces the old hardcoded 0.0001f which ran 2× faster at 96kHz.
        thermalState += thermalCoeff * (thermalTarget - thermalState);

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // Read LFO params once per block — rate/shape don't change sample-to-sample
        const float lfo1Rate = paramLfo1Rate ? paramLfo1Rate->load() : 0.5f;
        const float lfo1Depth = paramLfo1Depth ? paramLfo1Depth->load() : 0.0f;
        const int lfo1Shape = paramLfo1Shape ? static_cast<int>(paramLfo1Shape->load()) : 0;
        const float lfo2Rate = paramLfo2Rate ? paramLfo2Rate->load() : 1.0f;
        const float lfo2Depth = paramLfo2Depth ? paramLfo2Depth->load() : 0.0f;
        const int lfo2Shape = paramLfo2Shape ? static_cast<int>(paramLfo2Shape->load()) : 0;

        // Apply LFO rate/shape once per block per voice — not per sample.
        // F1: shimmerLFO rate is also block-level (pShimmerHz is block-constant);
        //     moved here from inside the sample loop where it fired every sample.
        const float shimmerLFORate = std::max(0.05f, pShimmerHz * 0.05f);
        // Apply LFO rate/shape once per block per voice — not per sample
        const float shimmerLfoRate = std::max(0.05f, pShimmerHz * 0.05f);
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);
            voice.shimmerLFO.setRate(shimmerLFORate, srf); // F1: block-level, not per-sample
            // Shimmer LFO rate depends only on pShimmerHz (block-constant); hoist.
            voice.shimmerLFO.setRate(shimmerLfoRate, srf);
        }

        // malletNow is block-constant (smoothMallet advances each sample but inside
        // the inner mode loop we use `1.5f - malletNow * 1.2f` — when collapsed as a
        // block-rate scalar this eliminates kMaxModes × numVoices × numSamples mul+sub.
        // We read it once per sample below but cache its derived falloff factor.

        for (int s = 0; s < numSamples; ++s)
        {
            float matNow = smoothMaterial.process();
            float malletNow = smoothMallet.process();
            float buzzNow = smoothBuzz.process();
            float bodyDNow = smoothBodyDepth.process();
            float sympNow = smoothSympathy.process();
            float brightNow = smoothBrightness.process();

            // Per-sample mallet falloff — used inside the per-mode inner loop below.
            // Hoisting (1.5 - malletNow * 1.2) here eliminates kMaxModes × kVoices
            // redundant computations per sample.
            const float malletFalloff = 1.5f - malletNow * 1.2f;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + capturedPitchMod);
                freq *= blockBendRatio; // hoisted; pre-reset pitch coupling snapshot

                float lfo1Val = voice.lfo1.process() * lfo1Depth; // LFO1 → brightness
                float lfo2Val = voice.lfo2.process() * lfo2Depth; // LFO2 → material

                // D004 FIX: apply LFO2 modulation to material continuum (per-voice, ±0.3 range).
                // Use a local voiceMatNow so each voice's LFO2 is independent.
                float voiceMatNow = std::clamp(matNow + lfo2Val * 0.3f, 0.0f, 1.0f);

                // Improvement #5: apply thermal drift (shared + per-voice personality)
                float totalThermalCents = thermalState + voice.thermalPersonality * pThermal * 0.5f;
                freq *= fastPow2(totalThermalCents * (1.0f / 1200.0f)); // BUG-3 FIX: fastPow2 replaces std::pow

                // Improvement #3: Balinese beat-frequency shimmer (fixed Hz, not ratio).
                // F1: shimmerLFO.setRate() moved to block-level pre-loop above.
                // F4: shimmerOffset applied only to mode 0 (fundamental) — NOT propagated
                //     through modal ratios. Balinese ombak is a fixed-Hz beat between
                //     paired bars tuned ~3-7 Hz apart; scaling it by high modal ratios
                //     (e.g., 9×) would produce semitone-class detuning at upper modes.
                // Improvement #3: Balinese beat-frequency shimmer (fixed Hz, not ratio)
                // BUG-2 FIX: use pShimmerHz parameter instead of hardcoded 0.3
                // (shimmerLFO.setRate hoisted to per-block voice loop above.)
                float shimmerMod = (voice.shimmerLFO.process() + 1.0f) * 0.5f;      // [0,1]
                float shimmerOffset = pShimmerHz * shimmerMod;                      // 0 to shimmerHz
                // Apply as additive Hz offset (Balinese: beat rate in Hz, not cents)
                float freqWithShimmer = freq + shimmerOffset;

                float excitation = voice.exciter.process();

                // CPU-optimized sympathetic resonance: use precomputed sparse table
                // instead of O(V²×M²) per-sample brute force. Table built at note-on.
                float sympInputPerMode[OwareVoice::kMaxModes] = {};
                if (sympNow > 0.001f)
                {
                    for (int ci = 0; ci < voice.sympathyCouplingCount; ++ci)
                    {
                        const auto& c = voice.sympathyCouplings[ci];
                        sympInputPerMode[c.thisModeIdx] +=
                            voices[c.otherVoiceIdx].modes[c.otherModeIdx].lastOutput * c.gain * sympNow;
                    }
                }

                // Modal resonator bank
                float resonanceSum = 0.0f;
                for (int m = 0; m < OwareVoice::kMaxModes; ++m)
                {
                    float woodR = kWoodRatios[m], metalR = kMetalRatios[m];
                    float bellR = kBellRatios[m];

                    float ratio;
                    if (voiceMatNow < 0.33f)
                        ratio = woodR + (bellR - woodR) * (voiceMatNow / 0.33f);
                    else if (voiceMatNow < 0.66f)
                        ratio = bellR + (metalR - bellR) * ((voiceMatNow - 0.33f) / 0.33f);
                    else
                        ratio = metalR + (kBowlRatios[m] - metalR) * ((voiceMatNow - 0.66f) / 0.34f);

                    // F4: shimmer applied only to mode 0 (fundamental pair detuning).
                    //     Upper modes use clean `freq` × ratio — Balinese ombak is a
                    //     fixed Hz offset between two bars, not a per-mode detuning.
                    float modeFreq = (m == 0 ? freqWithShimmer : freq) * ratio;

                    // Q: material-dependent base + mode-dependent falloff
                    float baseQ = 80.0f + voiceMatNow * 1420.0f;
                    float modeQ = baseQ / (1.0f + static_cast<float>(m) * 0.3f);

                    // Improvement #7: apply body-membrane coupling boost to Q
                    modeQ *= voice.modeDecayBoosts[m];

                    // D001: mallet hardness controls upper mode excitation.
                    // malletFalloff = (1.5f - malletNow * 1.2f) hoisted above the voice+mode
                    // loops — was recomputed M×V times per sample.
                    float modeAmp = 1.0f / (1.0f + static_cast<float>(m) * malletFalloff);

                    // Improvement #1: material exponent alpha — per-mode decay scaling.
                    // modeDecayScale[] precomputed once per block (materialAlpha × logModeIndex
                    // are both block-constant, so per-sample fastExp is no longer needed).
                    modeAmp *= modeDecayScale[m];

                    voice.modes[m].setFreqAndQ(modeFreq, modeQ, srf);
                    float modeInput = excitation + sympInputPerMode[m];
                    resonanceSum += voice.modes[m].process(modeInput) * modeAmp;
                }

                resonanceSum *= 0.25f;

                // Improvement #6: BPF-extracted buzz membrane
                float buzzed = voice.buzz.process(resonanceSum, buzzNow, pBodyType);

                voice.body.setFundamental(freq);
                float bodied = voice.body.process(buzzed, pBodyType, bodyDNow);

                // Amplitude envelope with material-aware decay.
                // noteOffDecayBoost > 1.0 during release — multiplied in to accelerate decay
                // smoothly over ~5ms rather than applying a hard amplitude step at noteOff.
                voice.ampLevel *= baseDecayCoeff * voice.noteOffDecayBoost;
                voice.ampLevel = flushDenormal(voice.ampLevel);
                if (voice.ampLevel < 1e-6f)
                {
                    voice.active = false;
                    continue;
                }

                // Tick filter envelope per sample; decimate SVF coeff refresh to every 16.
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 4000.0f;
                // BUG-1 FIX: LFO1 modulates brightness (±3000 Hz at full depth)
                // F3: svf.setMode is constant (always LowPass) — moved to noteOn; removed here.
                // F2: svf.setCoefficients is now guarded by a cached cutoff — only fires
                //     when cutoff changes by >0.1 Hz (eliminates expensive coeff recomputation
                //     on sustaining notes with static brightness/envelope).
                float cutoff = std::clamp(brightNow + envMod + lfo1Val * 3000.0f, 200.0f, 20000.0f);
                if (std::abs(cutoff - voice.lastCutoff) > 0.1f)
                {
                    voice.svf.setCoefficients(cutoff, 0.5f, srf);
                    voice.lastCutoff = cutoff;
                float filtered = voice.svf.processSample(bodied);
                // F10: voice.sympatheticOut removed — it was set here but never read
                //      (getSampleForCoupling exports couplingCacheL/R, not sympatheticOut).
                float output = filtered * voice.ampLevel;

                // BUG-3 FIX: use cached pan gains (computed at noteOn, not per-sample)
                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR)
                outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoiceCount.store(count);
        analyzeForSilenceGate(buffer, numSamples);
    } // end for (int s...) sample loop

    } // end renderBlock

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn(int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice(voices, kMaxVoices);
        auto& v = voices[idx];

        // F8: fastPow2 replaces std::pow for MIDI→Hz conversion (P18 pattern).
        float freq = 440.0f * fastPow2((static_cast<float>(note) - 69.0f) / 12.0f);
        int bodyType = paramBodyType ? static_cast<int>(paramBodyType->load()) : 0;

        // Read material once — used for filter decay AND body-membrane coupling boosts
        float matNow = paramMaterial ? paramMaterial->load() : 0.2f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);
        v.ampLevel = 1.0f;
        // F3: set SVF mode once at note-on (always LowPass) — not per-sample in renderBlock.
        v.svf.setMode(CytomicSVF::Mode::LowPass);
        v.lastCutoff = -1.0f; // force first-sample SVF coefficient computation

        // D001 + D006: velocity + aftertouch → mallet hardness
        float hardness = std::clamp((paramMalletHardness ? paramMalletHardness->load() : 0.3f) + vel * 0.5f +
                                        aftertouchAmount * 0.3f,
                                    0.0f, 1.0f);
        v.exciter.trigger(vel, hardness, freq, srf);

        v.filterEnv.prepare(srf);
        // D002 FIX: filter envelope decay varies with material position.
        // Wood (matNow≈0.0) = 100ms (fast, percussive), Bowl (matNow≈1.0) = 800ms (slow, singing).
        // Metal and Bell map through the continuum between these extremes.
        float filterDecay = 0.1f + matNow * 0.7f; // 100ms → 800ms across material
        v.filterEnv.setADSR(0.001f, filterDecay, 0.0f, 0.5f);
        v.filterEnv.triggerHard();

        v.shimmerLFO.reset(static_cast<float>(idx) / static_cast<float>(kMaxVoices));
        v.lfo1.reset();
        v.lfo2.reset();
        v.noteOffDecayBoost = 1.0f;

        for (auto& m : v.modes)
            m.reset();
        v.body.prepare(srf);
        v.buzz.prepare(srf, bodyType);

        // Improvement #7: compute body-membrane coupling boosts at note-on
        float bodyDepth = paramBodyDepth ? paramBodyDepth->load() : 0.5f;
        for (int m = 0; m < OwareVoice::kMaxModes; ++m)
        {
            float woodR = kWoodRatios[m], metalR = kMetalRatios[m], bellR = kBellRatios[m];
            float ratio;
            if (matNow < 0.33f)
                ratio = woodR + (bellR - woodR) * (matNow / 0.33f);
            else if (matNow < 0.66f)
                ratio = bellR + (metalR - bellR) * ((matNow - 0.33f) / 0.33f);
            else
                ratio = metalR + (kBowlRatios[m] - metalR) * ((matNow - 0.66f) / 0.34f);
            float modeFreq = freq * ratio;
            v.modeDecayBoosts[m] = v.body.getModeDecayBoost(modeFreq, bodyType, bodyDepth);
        }

        // CPU optimization: rebuild sparse sympathetic coupling tables for ALL active voices
        rebuildSympathyCouplingTables();
    }

    void noteOff(int note) noexcept
    {
        // ~5ms smooth release to avoid the -10.5dB step click of ampLevel *= 0.3f.
        // F7: releaseBoost precomputed in prepare() — std::exp(std::log(0.3f) / (0.005f * sr))
        //     is a constant for a given sample rate; computing it on every noteOff wastes cycles.
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.noteOffDecayBoost = noteOffReleaseBoost; // F7: use precomputed constant
                v.filterEnv.release();
            }
        }
        // Rebuild tables since voice activity changed
        rebuildSympathyCouplingTables();
    }

    /// CPU optimization: precompute sparse sympathetic coupling table for each voice.
    /// Called at note-on and note-off (when voice activity changes).
    /// Replaces O(V²×M²) per-sample iteration with O(K) lookup where K ≤ 32.
    void rebuildSympathyCouplingTables() noexcept
    {
        for (int vi = 0; vi < kMaxVoices; ++vi)
        {
            auto& v = voices[vi];
            v.sympathyCouplingCount = 0;
            if (!v.active)
                continue;

            for (int oi = 0; oi < kMaxVoices; ++oi)
            {
                if (oi == vi || !voices[oi].active)
                    continue;
                const auto& other = voices[oi];

                for (int m = 0; m < OwareVoice::kMaxModes; ++m)
                {
                    float thisFreq = v.modes[m].freq;
                    if (thisFreq < 20.0f)
                        continue;

                    for (int om = 0; om < OwareVoice::kMaxModes; ++om)
                    {
                        float otherFreq = other.modes[om].freq;
                        if (otherFreq < 20.0f)
                            continue;

                        float dist = std::fabs(thisFreq - otherFreq);
                        if (dist < 50.0f && v.sympathyCouplingCount < OwareVoice::kMaxCouplings)
                        {
                            float proximity = 1.0f - dist / 50.0f;
                            auto& c = v.sympathyCouplings[v.sympathyCouplingCount++];
                            c.otherVoiceIdx = oi;
                            c.otherModeIdx = om;
                            c.thisModeIdx = m;
                            // Guru Bin: gain 0.03 was inaudible. 0.10 makes sympathetic
                            // resonance justify its CPU cost — you can hear voices singing
                            // to each other when they share mode frequencies.
                            c.gain = proximity * 0.10f;
                        }
                    }
                }
            }
        }
    }

    //==========================================================================
    // Parameters — 26 total (23 original + thermal drift + shimmer Hz rename + LFOs)
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_material", 1}, "Oware Material",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_malletHardness", 1}, "Oware Mallet Hardness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"owr_bodyType", 1}, "Oware Body Type", 0, 3, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_bodyDepth", 1}, "Oware Body Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        // Guru Bin: buzz defaults to 0.15 — the mirliton is OWARE's most culturally
        // distinctive feature. Defaulting to 0.0 hid the engine's identity.
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_buzzAmount", 1}, "Oware Buzz Membrane",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_sympathyAmount", 1}, "Oware Sympathy",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Improvement #3: shimmer as beat frequency in Hz (Balinese gamelan model)
        // Guru Bin: 6.0 Hz is aggressive. 4.0 Hz sits in the sweet spot for
        // "ethereal gamelan" — traditional Balinese ombak range is 3-7 Hz.
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_shimmerRate", 1}, "Oware Shimmer Beat Hz",
                                              juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 4.0f));

        // Improvement #5: thermal drift
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_thermalDrift", 1}, "Oware Thermal Drift",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_brightness", 1}, "Oware Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_damping", 1}, "Oware Damping",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_decay", 1}, "Oware Decay",
                                              juce::NormalisableRange<float>(0.05f, 10.0f, 0.0f, 0.4f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_filterEnvAmount", 1}, "Oware Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_bendRange", 1}, "Oware Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_macroMaterial", 1}, "Oware Macro MATERIAL",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_macroMallet", 1}, "Oware Macro MALLET",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_macroCoupling", 1}, "Oware Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_macroSpace", 1}, "Oware Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_lfo1Rate", 1}, "Oware LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_lfo1Depth", 1}, "Oware LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f),
                                              0.1f)); // Seance: subtle breathing shimmer on first touch
        params.push_back(std::make_unique<PI>(juce::ParameterID{"owr_lfo1Shape", 1}, "Oware LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_lfo2Rate", 1}, "Oware LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"owr_lfo2Depth", 1}, "Oware LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"owr_lfo2Shape", 1}, "Oware LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramMaterial = apvts.getRawParameterValue("owr_material");
        paramMalletHardness = apvts.getRawParameterValue("owr_malletHardness");
        paramBodyType = apvts.getRawParameterValue("owr_bodyType");
        paramBodyDepth = apvts.getRawParameterValue("owr_bodyDepth");
        paramBuzzAmount = apvts.getRawParameterValue("owr_buzzAmount");
        paramSympathyAmount = apvts.getRawParameterValue("owr_sympathyAmount");
        paramShimmerRate = apvts.getRawParameterValue("owr_shimmerRate");
        paramThermalDrift = apvts.getRawParameterValue("owr_thermalDrift");
        paramBrightness = apvts.getRawParameterValue("owr_brightness");
        paramDamping = apvts.getRawParameterValue("owr_damping");
        paramDecay = apvts.getRawParameterValue("owr_decay");
        paramFilterEnvAmount = apvts.getRawParameterValue("owr_filterEnvAmount");
        paramBendRange = apvts.getRawParameterValue("owr_bendRange");
        paramMacroMaterial = apvts.getRawParameterValue("owr_macroMaterial");
        paramMacroMallet = apvts.getRawParameterValue("owr_macroMallet");
        paramMacroCoupling = apvts.getRawParameterValue("owr_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("owr_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("owr_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("owr_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("owr_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("owr_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("owr_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("owr_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OwareVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothMaterial, smoothMallet, smoothBuzz;
    ParameterSmoother smoothBodyDepth, smoothSympathy, smoothBrightness;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // Improvement #5: thermal drift state
    float thermalState = 0.0f, thermalTarget = 0.0f;
    float thermalCoeff = 0.0001f; // sample-rate-scaled in prepare(); fallback matches old 48kHz value
    int thermalTimer = 0;
    uint32_t thermalNoiseState = 54321u;

    // F7: precomputed noteOff release coefficient — avoid std::exp on every MIDI event.
    float noteOffReleaseBoost = 0.999f; // fallback; correct value set in prepare()

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingMaterialMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramMaterial = nullptr;
    std::atomic<float>* paramMalletHardness = nullptr;
    std::atomic<float>* paramBodyType = nullptr;
    std::atomic<float>* paramBodyDepth = nullptr;
    std::atomic<float>* paramBuzzAmount = nullptr;
    std::atomic<float>* paramSympathyAmount = nullptr;
    std::atomic<float>* paramShimmerRate = nullptr;
    std::atomic<float>* paramThermalDrift = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramMacroMaterial = nullptr;
    std::atomic<float>* paramMacroMallet = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;
};

} // namespace xoceanus
