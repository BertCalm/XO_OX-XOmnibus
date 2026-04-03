// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// FatEngine — OBESE multi-oscillator synth adapted from XObese.
//
// 13 oscillators per voice: 1 sub + 4 groups of 3 (root, +12st, -12st).
// Each group → ZDF Ladder Filter → Stereo Pan → Amp Envelope → Output.
// Mojo system: per-oscillator analog drift + soft-clip.
// Built-in arpeggiator, saturation, and bitcrusher.
//
// Accent: Hot Pink #FF1493
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace xoceanus
{

// FastMath functions (flushDenormal, fastTanh, fastSin, fastExp, clamp, lerp)
// are free functions in the xoceanus namespace, included via FastMath.h.

//==============================================================================
// FatNoiseGen — xorshift32 PRNG for noise oscillator.
//==============================================================================
class FatNoiseGen
{
public:
    void seed(uint32_t s) noexcept { state = s ? s : 1; }

    float next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(static_cast<int32_t>(state)) * (1.0f / 2147483648.0f);
    }

private:
    uint32_t state = 12345;
};

//==============================================================================
// FatMorphOsc — Morphable oscillator with inline PolyBLEP anti-aliasing.
//
// Morph: 0=Sine, 0.33=Saw, 0.66=Square, 1.0=Noise
// Crossfades between adjacent waveforms. Noise uses xorshift32.
//==============================================================================
class FatMorphOsc
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        phase = 0.0;
        phaseInc = 0.0;
    }

    void reset() noexcept { phase = 0.0; }

    void setFrequency(double freq) noexcept { phaseInc = clamp(static_cast<float>(freq), 20.0f, 20000.0f) / sr; }

    float process(float morph, FatNoiseGen& noise) noexcept
    {
        float t = static_cast<float>(phase);
        float out;

        if (morph <= 0.33f)
        {
            // Sine → Saw blend
            float sine = fastSin(t * 6.2831853f);
            float saw = 2.0f * t - 1.0f - polyBLEP(phase, phaseInc);
            out = lerp(sine, saw, morph * 3.0303f); // 1/0.33
        }
        else if (morph <= 0.66f)
        {
            // Saw → Square blend
            float saw = 2.0f * t - 1.0f - polyBLEP(phase, phaseInc);
            float sqr = (t < 0.5f ? 1.0f : -1.0f);
            sqr += polyBLEP(phase, phaseInc);
            double p2 = phase + 0.5;
            if (p2 >= 1.0)
                p2 -= 1.0;
            sqr -= polyBLEP(p2, phaseInc);
            out = lerp(saw, sqr, (morph - 0.33f) * 3.0303f);
        }
        else
        {
            // Square → Noise blend
            float sqr = (t < 0.5f ? 1.0f : -1.0f);
            sqr += polyBLEP(phase, phaseInc);
            double p2 = phase + 0.5;
            if (p2 >= 1.0)
                p2 -= 1.0;
            sqr -= polyBLEP(p2, phaseInc);
            float noiseSample = noise.next();
            float blend = (morph - 0.66f) * 2.9412f; // 1/0.34
            out = lerp(sqr, noiseSample, blend);
        }

        // Advance phase
        phase += phaseInc;
        if (phase >= 1.0)
            phase -= 1.0;

        return out;
    }

    // Process as sub oscillator (triangle, no morph needed)
    float processSub() noexcept
    {
        float t = static_cast<float>(phase);
        float tri = 4.0f * std::abs(t - 0.5f) - 1.0f;

        phase += phaseInc;
        if (phase >= 1.0)
            phase -= 1.0;

        return tri;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;

    static float polyBLEP(double t, double dt) noexcept
    {
        if (dt <= 0.0)
            return 0.0f;
        if (t < dt)
        {
            float tn = static_cast<float>(t / dt);
            return tn + tn - tn * tn - 1.0f;
        }
        if (t > 1.0 - dt)
        {
            float tn = static_cast<float>((t - 1.0) / dt);
            return tn * tn + tn + tn + 1.0f;
        }
        return 0.0f;
    }
};

//==============================================================================
// FatMojoDrift — Per-oscillator smooth random walk for analog character.
// Each oscillator gets unique drift (±3 cents max).
//==============================================================================
class FatMojoDrift
{
public:
    void prepare(double sampleRate) noexcept { sr = sampleRate; }

    void seed(uint32_t s) noexcept { rng.seed(s); }

    void reset() noexcept { smoothed = 0.0f; }

    // Returns pitch drift in cents, scaled by mojoAmount
    float process(float mojoAmount) noexcept
    {
        if (mojoAmount < 0.001f)
        {
            smoothed = 0.0f;
            return 0.0f;
        }

        // Slow random walk: new target every ~100ms
        counter += 1.0f;
        float rate = 10.0f; // ~10 Hz random walk
        if (counter >= static_cast<float>(sr) / rate)
        {
            counter = 0.0f;
            target = rng.next() * 6.0f - 3.0f; // ±3 cents
        }

        // Smooth toward target
        float coeff = 0.0001f;
        smoothed += coeff * (target - smoothed);
        smoothed = flushDenormal(smoothed);

        return smoothed * mojoAmount;
    }

private:
    double sr = 44100.0;
    FatNoiseGen rng;
    float smoothed = 0.0f;
    float target = 0.0f;
    float counter = 0.0f;
};

//==============================================================================
// FatLadderFilter — ZDF 4-pole ladder lowpass with drive.
// Proper tan() pre-warp, resonance scaling near Nyquist, inter-stage soft-clip.
//==============================================================================
class FatLadderFilter
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float>(sr);
        s[0] = s[1] = s[2] = s[3] = 0.0f;
        coeffDirty = true;
    }

    void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    void setCutoff(float hz) noexcept
    {
        if (hz != cutoffHz)
        {
            cutoffHz = hz;
            coeffDirty = true;
        }
    }

    void setResonance(float r) noexcept
    {
        if (r != resonance)
        {
            resonance = r;
            coeffDirty = true;
        }
    }

    void setDrive(float d) noexcept { drive = d; }

    // Pre-computed coefficient bundle — avoids redundant std::tan when
    // multiple filters in a voice share the same cutoff/resonance.
    struct Coeffs
    {
        float g, G, k;
    };

    // Compute coefficients once per voice per sample, share across all 4 filters.
    static Coeffs computeCoeffs(float hz, float reso, float invSR, float srF) noexcept
    {
        constexpr float pi = 3.14159265f;
        float fc = clamp(hz, 20.0f, srF * 0.42f);
        float gv = std::tan(pi * fc * invSR);
        float Gv = gv / (1.0f + gv);
        float nyqRatio = fc / srF;
        float resoScale = (nyqRatio < 0.3f) ? 1.0f : 1.0f - 0.7f * ((nyqRatio - 0.3f) / 0.12f);
        resoScale = clamp(resoScale, 0.3f, 1.0f);
        return {gv, Gv, std::min(4.0f * reso * resoScale, 3.2f)};
    }

    // Apply pre-computed coefficients — skips std::tan.
    void applyCoeffs(const Coeffs& c) noexcept
    {
        g = c.g;
        G = c.G;
        k = c.k;
        coeffDirty = false;
    }

    float processSample(float input) noexcept
    {
        updateCoefficients(); // no-op if called after applyCoeffs()

        // Drive: pre-saturate input
        float driveGain = 1.0f + drive * 3.0f;
        float driven = fastTanh(input * driveGain);

        // Feedback from 4th pole (delayed by 1 sample)
        float fb = fastTanh(s[3]);
        float u = driven - k * fb;

        // 4 cascaded 1-pole ZDF lowpass
        for (int i = 0; i < 4; ++i)
        {
            float v = G * (u - s[i]);
            float lp = v + s[i];
            s[i] = lp + v;
            s[i] = flushDenormal(s[i]);
            lp = fastTanh(lp); // inter-stage soft-clip
            u = lp;
        }

        if (!std::isfinite(u))
            u = 0.0f;
        return clamp(u, -4.0f, 4.0f);
    }

private:
    double sr = 44100.0;
    float invSR = 1.0f / static_cast<float>(sr); // overwritten by prepare()
    float cutoffHz = 2000.0f;
    float resonance = 0.2f;
    float drive = 0.0f;
    bool coeffDirty = true;

    float g = 0.0f, G = 0.0f, k = 0.0f;
    float s[4] = {};

    void updateCoefficients() noexcept
    {
        if (!coeffDirty)
            return;
        coeffDirty = false;

        constexpr float pi = 3.14159265f;
        float fc = clamp(cutoffHz, 20.0f, static_cast<float>(sr) * 0.42f);
        g = std::tan(pi * fc * invSR);
        G = g / (1.0f + g);

        // Resonance scaling: reduce near Nyquist for stability
        float nyqRatio = fc / static_cast<float>(sr);
        float resoScale = (nyqRatio < 0.3f) ? 1.0f : 1.0f - 0.7f * ((nyqRatio - 0.3f) / 0.12f);
        resoScale = clamp(resoScale, 0.3f, 1.0f);
        k = std::min(4.0f * resonance * resoScale, 3.2f);
    }
};

// FatEnvelope replaced by shared StandardADSR (Source/DSP/StandardADSR.h).
// FatVoice::ampEnv and FatVoice::filterEnv are now StandardADSR instances.

//==============================================================================
// FatSaturation — Asymmetric waveshaper + DC blocker.
//==============================================================================
class FatSaturation
{
public:
    void prepare(double /*sampleRate*/) noexcept
    {
        dcPrev = 0.0f;
        dcOut = 0.0f;
    }
    void reset() noexcept
    {
        dcPrev = 0.0f;
        dcOut = 0.0f;
    }

    void setDrive(float driveAmount) noexcept
    {
        if (driveAmount != lastDrive)
        {
            lastDrive = driveAmount;
            float gain = 1.0f + driveAmount * 7.0f;
            cachedGain = gain;
            cachedOutputGain = 1.0f / std::sqrt(gain);
        }
    }

    float process(float input) noexcept
    {
        if (lastDrive < 0.001f)
            return input;

        float driven = input * cachedGain;

        // Asymmetric saturation: positive clips harder → even harmonics
        float wet = fastTanh(driven) * 0.7f + fastTanh(driven * 0.5f + 0.3f) * 0.3f;
        wet *= cachedOutputGain;

        // DC blocker (~5 Hz highpass)
        float dcIn = wet;
        dcOut = dcIn - dcPrev + 0.9995f * dcOut;
        dcPrev = dcIn;
        dcOut = flushDenormal(dcOut);

        return lerp(input, dcOut, lastDrive);
    }

private:
    float dcPrev = 0.0f;
    float dcOut = 0.0f;
    float lastDrive = -1.0f;
    float cachedGain = 1.0f;
    float cachedOutputGain = 1.0f;
};

//==============================================================================
// FatBitcrusher — Sample-rate reduction + bit-depth quantization.
//==============================================================================
class FatBitcrusher
{
public:
    void prepare(double sampleRate) noexcept
    {
        hostSR = sampleRate;
        phaseAcc = 0.0f;
        held = 0.0f;
    }
    void reset() noexcept
    {
        phaseAcc = 0.0f;
        held = 0.0f;
        rng = 0x1234abcdu;
    }

    // Process with precomputed levels/invLevels to avoid per-sample std::pow.
    // levels = std::pow(2.0f, std::floor(bitDepth)) - 1.0f (block-constant)
    // invLevels = 1.0f / levels
    // TPDF dither is added before quantization to break up quantization harmonics.
    float process(float input, float crushRate, float levels, float invLevels) noexcept
    {
        // Bypass if at full quality (levels == 65535 → bitDepth == 16)
        if (levels >= 65535.0f && crushRate >= static_cast<float>(hostSR) - 1.0f)
            return input;

        // Sample-and-hold
        phaseAcc += crushRate / static_cast<float>(hostSR);
        if (phaseAcc >= 1.0f)
        {
            phaseAcc -= 1.0f;

            // TPDF dither: two uniform noise samples subtracted → triangular distribution
            // amplitude = ±1 LSB, spectrally flat, decorrelates quantization error
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5; // xorshift32
            const uint32_t r1 = rng;
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            const uint32_t r2 = rng;
            // Map [0, 2^31) to [-0.5, 0.5) for each, difference gives [-1, 1)
            const float dither = (static_cast<float>(r1 >> 1) - static_cast<float>(r2 >> 1)) *
                                 (invLevels / static_cast<float>(0x40000000u));

            held = std::round((input + dither) * levels) * invLevels;
        }

        return held;
    }

private:
    double hostSR = 44100.0;
    float phaseAcc = 0.0f;
    float held = 0.0f;
    uint32_t rng = 0x1234abcdu;
};

//==============================================================================
// FatArpeggiator — Pattern-based arpeggiator with internal tempo.
//
// Patterns: Up, Down, UpDown, Random, AsPlayed
// Rates: 1/4, 1/8, 1/8T, 1/16, 1/32
// Octave expansion: 1-3 octaves
//==============================================================================
class FatArpeggiator
{
public:
    static constexpr int kMaxHeldNotes = 16;

    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        numHeld = 0;
        stepIndex = 0;
        sampleCounter = 0;
        gateCounter = 0;
        noteActive = false;
        currentArpNote = -1;
    }

    void noteOn(int note, float velocity) noexcept
    {
        // Add to held list if not already present
        for (int i = 0; i < numHeld; ++i)
            if (heldNotes[i] == note)
                return;
        if (numHeld < kMaxHeldNotes)
        {
            heldNotes[numHeld] = note;
            heldVelocities[numHeld] = velocity;
            ++numHeld;
        }
    }

    void noteOff(int note) noexcept
    {
        for (int i = 0; i < numHeld; ++i)
        {
            if (heldNotes[i] == note)
            {
                for (int j = i; j < numHeld - 1; ++j)
                {
                    heldNotes[j] = heldNotes[j + 1];
                    heldVelocities[j] = heldVelocities[j + 1];
                }
                --numHeld;
                break;
            }
        }
    }

    struct ArpEvent
    {
        int note;
        float velocity;
        bool isNoteOn;
    };

    // Process one block: returns up to 2 events (note-off + note-on)
    // Call once per block with block size
    void processBlock(int numSamples, float bpm, int rateIndex, int patternIndex, int octaves, float gate,
                      ArpEvent* events, int& numEvents) noexcept
    {
        numEvents = 0;
        if (numHeld == 0)
        {
            if (noteActive)
            {
                events[numEvents++] = {currentArpNote, 0.0f, false};
                noteActive = false;
                currentArpNote = -1;
            }
            return;
        }

        static constexpr float rateMultipliers[] = {1.0f, 0.5f, 0.3333f, 0.25f, 0.125f};
        float rateMul = rateMultipliers[std::min(std::max(rateIndex, 0), 4)];
        int samplesPerStep = static_cast<int>((60.0 / std::max(bpm, 40.0f)) * sr * rateMul);
        int gateSamples = static_cast<int>(static_cast<float>(samplesPerStep) * gate);

        sampleCounter += numSamples;

        // Gate off?
        if (noteActive)
        {
            gateCounter += numSamples;
            if (gateCounter >= gateSamples)
            {
                events[numEvents++] = {currentArpNote, 0.0f, false};
                noteActive = false;
            }
        }

        // Next step?
        if (sampleCounter >= samplesPerStep)
        {
            sampleCounter -= samplesPerStep;
            gateCounter = 0;

            // Generate pattern index
            int totalNotes = numHeld * octaves;
            int patIdx = getPatternIndex(patternIndex, totalNotes);

            int noteIdx = patIdx % numHeld;
            int octShift = patIdx / numHeld;

            int note = heldNotes[noteIdx] + octShift * 12;
            float vel = heldVelocities[noteIdx];

            if (noteActive && currentArpNote != note)
                events[numEvents++] = {currentArpNote, 0.0f, false};

            events[numEvents++] = {note, vel, true};
            currentArpNote = note;
            noteActive = true;

            advanceStep(patternIndex, totalNotes);
        }
    }

    bool hasHeldNotes() const noexcept { return numHeld > 0; }

private:
    double sr = 44100.0;
    int heldNotes[kMaxHeldNotes] = {};
    float heldVelocities[kMaxHeldNotes] = {};
    int numHeld = 0;
    int stepIndex = 0;
    int sampleCounter = 0;
    int gateCounter = 0;
    bool noteActive = false;
    int currentArpNote = -1;
    bool ascending = true; // for UpDown pattern

    int getPatternIndex(int pattern, int total) noexcept
    {
        if (total <= 0)
            return 0;
        switch (pattern)
        {
        case 0:
            return stepIndex % total; // Up
        case 1:
            return (total - 1 - stepIndex % total); // Down
        case 2:
            return ascending ? (stepIndex % total) // UpDown
                             : (total - 1 - stepIndex % total);
        case 3: // Random
        {
            rngState ^= rngState << 13;
            rngState ^= rngState >> 17;
            rngState ^= rngState << 5;
            return static_cast<int>(rngState % static_cast<uint32_t>(total));
        }
        case 4:
            return stepIndex % total; // AsPlayed
        default:
            return 0;
        }
    }

    void advanceStep(int pattern, int total) noexcept
    {
        ++stepIndex;
        if (pattern == 2 && total > 1) // UpDown
        {
            if (ascending && stepIndex >= total)
            {
                ascending = false;
                stepIndex = 0;
            }
            else if (!ascending && stepIndex >= total)
            {
                ascending = true;
                stepIndex = 0;
            }
        }
    }

    uint32_t rngState = 42;
};

//==============================================================================
// FatVoice — 13-oscillator voice with 4 filter groups.
//
// Osc 0:       Sub (triangle, configurable octave offset)
// Oscs 1-3:    Group 1 → Filter 1 → center pan
// Oscs 4-6:    Group 2 → Filter 2 → left pan
// Oscs 7-9:    Group 3 → Filter 3 → right pan
// Oscs 10-12:  Group 4 → Filter 4 → stereo wide
//==============================================================================
struct FatVoice
{
    bool active = false;
    int noteNumber = 60;
    float velocity = 0.0f;
    uint64_t age = 0;       // samples elapsed since note-on (used in render loop)
    uint64_t startTime = 0; // monotonic timestamp at note-on (used by VoiceAllocator LRU)

    // Sub oscillator
    FatMorphOsc subOsc;
    FatMojoDrift subDrift;

    // 12 main oscillators (4 groups × 3)
    std::array<FatMorphOsc, 12> oscs;
    std::array<FatMojoDrift, 12> drifts;
    std::array<FatNoiseGen, 12> noiseGens;
    FatNoiseGen subNoise;

    // 4 ladder filters (one per group)
    std::array<FatLadderFilter, 4> filters;

    // Envelopes — shared StandardADSR (Source/DSP/StandardADSR.h)
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // D005: Breathing LFO — shared BreathingLFO (Source/DSP/StandardLFO.h)
    BreathingLFO breathingLFO;

    // Saturation per voice
    FatSaturation saturation;
    FatBitcrusher crusher;

    // Glide
    bool glideActive = false;
    float glideSourceFreq = 0.0f;
    float cachedBaseFreq = 261.63f;

    // Sustain pedal hold: voice stays active while CC64 is down
    bool sustainHeld = false;
};

//==============================================================================
// FatEngine — Multi-oscillator synth engine (from XObese).
//==============================================================================
class FatEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 6;

    //-- Identity ---------------------------------------------------------------
    juce::String getEngineId() const override { return "Obese"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFFF1493); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(std::memory_order_relaxed); }

    //-- Lifecycle --------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);
        silenceGate.prepare(sampleRate, maxBlockSize);

        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        arp.prepare(sampleRate);
        aftertouch.prepare(sampleRate); // D006: 5ms attack / 20ms release smoothing

        for (int v = 0; v < kMaxVoices; ++v)
        {
            auto& voice = voices[static_cast<size_t>(v)];
            voice.subOsc.prepare(sampleRate);
            voice.subDrift.prepare(sampleRate);
            voice.subDrift.seed(static_cast<uint32_t>(v * 7919 + 42));
            voice.subNoise.seed(static_cast<uint32_t>(v * 3001 + 7));

            for (int i = 0; i < 12; ++i)
            {
                voice.oscs[static_cast<size_t>(i)].prepare(sampleRate);
                voice.drifts[static_cast<size_t>(i)].prepare(sampleRate);
                voice.drifts[static_cast<size_t>(i)].seed(static_cast<uint32_t>(v * 7919 + i * 131 + 42));
                voice.noiseGens[static_cast<size_t>(i)].seed(static_cast<uint32_t>(v * 5003 + i * 271 + 99));
            }
            for (int g = 0; g < 4; ++g)
                voice.filters[static_cast<size_t>(g)].prepare(sampleRate);
            voice.ampEnv.prepare(static_cast<float>(sampleRate));
            voice.filterEnv.prepare(static_cast<float>(sampleRate));
            voice.breathingLFO.setRate(0.07f, static_cast<float>(sampleRate)); // D005: 0.07 Hz autonomous breathing
            voice.saturation.prepare(sampleRate);
            voice.crusher.prepare(sampleRate);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        arp.reset();
        for (auto& v : voices)
        {
            v.active = false;
            v.subOsc.reset();
            v.subDrift.reset();
            for (int i = 0; i < 12; ++i)
            {
                v.oscs[static_cast<size_t>(i)].reset();
                v.drifts[static_cast<size_t>(i)].reset();
            }
            for (int g = 0; g < 4; ++g)
                v.filters[static_cast<size_t>(g)].reset();
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.saturation.reset();
            v.crusher.reset();
        }
    }

    //-- Audio ------------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // -- XOceanus macros (MOJO, GRIT, SIZE, CRUSH) — read first, used below ---
        const float macroMojo = (pMacroMojo != nullptr) ? pMacroMojo->load() : 0.0f;
        const float macroGrit = (pMacroGrit != nullptr) ? pMacroGrit->load() : 0.0f;
        const float macroSize = (pMacroSize != nullptr) ? pMacroSize->load() : 0.0f;
        const float macroCrush = (pMacroCrush != nullptr) ? pMacroCrush->load() : 0.0f;

        // Snapshot parameters (block-constant)
        const float morph = (pMorph != nullptr) ? pMorph->load() : 0.5f;
        const float mojo = (pMojo != nullptr) ? pMojo->load() : 0.5f;
        const float subLevel = (pSubLevel != nullptr) ? pSubLevel->load() : 0.5f;
        const int subOct = (pSubOct != nullptr) ? static_cast<int>(pSubOct->load()) : 1;
        // M3 SIZE macro: adds group mix (+0.3) for wider octave stacks
        const float groupMix =
            clamp(((pGroupMix != nullptr) ? pGroupMix->load() : 1.0f) + macroSize * 0.3f, 0.0f, 1.0f);
        const float detune = (pDetune != nullptr) ? pDetune->load() : 5.0f;
        const float stereoWidth = (pStereoWidth != nullptr) ? pStereoWidth->load() : 0.5f;

        const float ampA = (pAmpAttack != nullptr) ? pAmpAttack->load() : 0.005f;
        const float ampD = (pAmpDecay != nullptr) ? pAmpDecay->load() : 0.2f;
        const float ampS = (pAmpSustain != nullptr) ? pAmpSustain->load() : 0.8f;
        const float ampR = (pAmpRelease != nullptr) ? pAmpRelease->load() : 0.15f;

        // M3 SIZE macro: opens filter cutoff (+6000 Hz) for massive stacks
        const float fltCutoff =
            clamp(((pFltCutoff != nullptr) ? pFltCutoff->load() : 2000.0f) + macroSize * 6000.0f, 20.0f, 18000.0f);
        const float fltReso = (pFltReso != nullptr) ? pFltReso->load() : 0.2f;
        // M2 GRIT macro: increases filter drive (+0.3) for more inter-stage saturation
        const float fltDrive =
            clamp(((pFltDrive != nullptr) ? pFltDrive->load() : 0.0f) + macroGrit * 0.3f, 0.0f, 1.0f);
        const float fltKeyTrack = (pFltKeyTrack != nullptr) ? pFltKeyTrack->load() : 0.5f;
        const float fltEnvAmt = (pFltEnvAmt != nullptr) ? pFltEnvAmt->load() : 0.5f;
        const float fltEnvA = (pFltEnvAttack != nullptr) ? pFltEnvAttack->load() : 0.001f;
        const float fltEnvD = (pFltEnvDecay != nullptr) ? pFltEnvDecay->load() : 0.3f;

        // M2 GRIT macro: increases saturation drive (+0.6) and filter drive (+0.3)
        const float satDrive =
            clamp(((pSatDrive != nullptr) ? pSatDrive->load() : 0.3f) + macroGrit * 0.6f, 0.0f, 1.0f);
        // M4 CRUSH macro: reduces crush depth and rate for lo-fi character
        const float crushDepth =
            clamp(((pCrushDepth != nullptr) ? pCrushDepth->load() : 16.0f) - macroCrush * 8.0f, 2.0f, 16.0f);
        const float crushRate =
            clamp(((pCrushRate != nullptr) ? pCrushRate->load() : srf) - macroCrush * 20000.0f, 500.0f, srf);

        const int arpOn = (pArpOn != nullptr) ? static_cast<int>(pArpOn->load()) : 0;
        const int arpPattern = (pArpPattern != nullptr) ? static_cast<int>(pArpPattern->load()) : 0;
        const int arpRate = (pArpRate != nullptr) ? static_cast<int>(pArpRate->load()) : 1;
        const int arpOctaves = (pArpOctaves != nullptr) ? static_cast<int>(pArpOctaves->load()) + 1 : 1;
        const float arpGate = (pArpGate != nullptr) ? pArpGate->load() : 0.5f;
        const float arpTempo = (pArpTempo != nullptr) ? pArpTempo->load() : 120.0f;

        const float level = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const int voiceMode = (pVoiceMode != nullptr) ? static_cast<int>(pVoiceMode->load()) : 0;
        const float glideAmt = (pGlide != nullptr) ? pGlide->load() : 0.0f;
        const int maxPoly = (pPolyphony != nullptr)
                                ? std::array<int, 4>{{1, 2, 4, 6}}[std::min(3, static_cast<int>(pPolyphony->load()))]
                                : 6;

        // D002: LFO1 — user-controllable BreathingLFO.
        // Target 0=Filter (breathes filter cutoff), 1=Mojo (B015 dynamic axis), 2=Saturation.
        // Rate floor 0.005 Hz satisfies D005.
        const float lfo1Rate = (pLfo1Rate != nullptr) ? pLfo1Rate->load() : 0.07f;
        const float lfo1Depth = (pLfo1Depth != nullptr) ? pLfo1Depth->load() : 0.15f;
        const int lfo1Target = (pLfo1Target != nullptr) ? static_cast<int>(pLfo1Target->load()) : 0;
        // Update BreathingLFO rate per-block for all voices (rate is block-constant).
        for (auto& voice : voices)
            voice.breathingLFO.setRate(lfo1Rate, static_cast<float>(sr));

        // D002: 2nd LFO — read once per block, advance block-rate.
        // Saw waveform (rising ramp) creates a characteristic gliding filter sweep.
        // Rate floor 0.005 Hz satisfies D005. Depth scales ±24 semitones at max (×fltCutoff).
        const float lfo2Rate = (pLfo2Rate != nullptr) ? pLfo2Rate->load() : 0.08f;
        const float lfo2Depth = (pLfo2Depth != nullptr) ? pLfo2Depth->load() : 0.25f;
        lfo2Phase += static_cast<double>(lfo2Rate) / sr * static_cast<double>(numSamples);
        if (lfo2Phase >= 1.0)
            lfo2Phase -= 1.0;
        // Saw wave: bipolar [-1, +1], rising over the full cycle period
        lfo2Output = static_cast<float>(2.0 * lfo2Phase - 1.0);
        // ±24 semitones = factor of 2× at max. Map depth [0,1] → [0, 24st].
        // Applied as additive semitone offset before the exponential cutoff computation.
        const float lfo2SemitonesMod = lfo2Output * lfo2Depth * 24.0f;

        // Sub octave: -2, -1, 0 → semitone offsets
        const float subSemitones = static_cast<float>((subOct - 2) * 12); // index 0=-24, 1=-12, 2=0

        // Analog amount: mojo=0 → digital, mojo=1 → full analog drift
        // M1 MOJO macro pushes toward analog (+0.5 at full throw)
        // D006: non-const so aftertouch boost (computed post-MIDI-loop) can update it
        float analogAmount = clamp(mojo + macroMojo * 0.5f, 0.0f, 1.0f);

        // Precompute group pan gains (constant-power)
        const float g2Pan = -0.3f * stereoWidth * 2.0f;
        const float g3Pan = +0.3f * stereoWidth * 2.0f;
        const float g4PanL = -1.0f * stereoWidth * 2.0f;
        const float g4PanR = +1.0f * stereoWidth * 2.0f;

        // Pan law: L = sqrt(0.5*(1-pan)), R = sqrt(0.5*(1+pan))
        auto panGains = [](float pan) -> std::pair<float, float>
        {
            pan = clamp(pan, -1.0f, 1.0f);
            return {std::sqrt(0.5f * (1.0f - pan)), std::sqrt(0.5f * (1.0f + pan))};
        };

        auto [g1L, g1R] = panGains(0.0f);
        auto [g2L, g2R] = panGains(g2Pan);
        auto [g3L, g3R] = panGains(g3Pan);
        // G4: wide stereo — oscs 10,11 left, osc 12 right
        auto [g4aL, g4aR] = panGains(g4PanL);
        auto [g4bL, g4bR] = panGains(g4PanR);

        // Precompute glide coefficient
        float glideCoeff = 0.0f;
        if (glideAmt > 0.0f)
        {
            float glideTimeSec = 0.005f + glideAmt * 0.495f;
            glideCoeff = 1.0f - std::exp(-1.0f / (srf * glideTimeSec));
        }

        // --- Process MIDI (arp or direct) ---
        if (arpOn)
        {
            // Feed MIDI to arpeggiator
            for (const auto metadata : midi)
            {
                const auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                    arp.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff())
                    arp.noteOff(msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                    arp.reset();
                // D006: CC1 mod wheel → Mojo boost (classic Moog expression axis; sensitivity 0.5)
                else if (msg.isController() && msg.getControllerNumber() == 1)
                    modWheelValue = msg.getControllerValue() / 127.0f;
                else if (msg.isPitchWheel())
                    pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }

            // Get arp events for this block
            FatArpeggiator::ArpEvent arpEvents[4];
            int numArpEvents = 0;
            arp.processBlock(numSamples, arpTempo, arpRate, arpPattern, arpOctaves, arpGate, arpEvents, numArpEvents);

            for (int i = 0; i < numArpEvents; ++i)
            {
                if (arpEvents[i].isNoteOn)
                {
                    silenceGate.wake();
                    noteOn(arpEvents[i].note, arpEvents[i].velocity, glideAmt, voiceMode, maxPoly);
                }
                else
                    noteOff(arpEvents[i].note);
            }
        }
        else
        {
            for (const auto metadata : midi)
            {
                const auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                {
                    silenceGate.wake();
                    noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), glideAmt, voiceMode, maxPoly);
                }
                else if (msg.isNoteOff())
                    noteOff(msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                    allVoicesOff();
                else if (msg.isController() && msg.getControllerNumber() == 64)
                {
                    bool wasDown = sustainPedalDown;
                    sustainPedalDown = (msg.getControllerValue() >= 64);
                    // On release: trigger noteOff for any sustain-held voices
                    if (wasDown && !sustainPedalDown)
                    {
                        for (auto& v : voices)
                        {
                            if (v.sustainHeld)
                            {
                                v.ampEnv.noteOff();
                                v.sustainHeld = false;
                            }
                        }
                    }
                }
                // D006: channel pressure → aftertouch (applied to mojo control below)
                else if (msg.isChannelPressure())
                    aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
                // D006: CC1 mod wheel → Mojo boost (classic Moog expression axis; sensitivity 0.5)
                else if (msg.isController() && msg.getControllerNumber() == 1)
                    modWheelValue = msg.getControllerValue() / 127.0f;
                else if (msg.isPitchWheel())
                    pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0); // channel-mode: voice 0 holds global value

        // D006: aftertouch pushes mojo toward analog end of spectrum (sensitivity 0.3).
        // Higher pressure = more analog drift + soft-clip saturation on all oscillators.
        // This is Blessing B015 in action: the Mojo orthogonal axis becomes touch-sensitive.
        // D006: mod wheel also boosts mojo up to +0.5 at full wheel (classic Moog expression; sensitivity 0.5)
        // M1 MOJO macro already added to analogAmount above; aftertouch + mod wheel also contribute
        const float effectiveMojo =
            clamp(mojo + macroMojo * 0.5f + atPressure * 0.3f + modWheelValue * 0.5f, 0.0f, 1.0f);
        analogAmount = effectiveMojo; // update after MIDI loop once atPressure is known

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;

        // --- Pre-compute per-voice block constants ---
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.ampEnv.setADSR(ampA, ampD, ampS, ampR);
            voice.filterEnv.setADSR(fltEnvA, fltEnvD, 0.0f, 0.5f);
            voice.cachedBaseFreq = midiToFreq(voice.noteNumber);
        }

        // Block-constant precomputes — hoisted out of the sample + voice loops.
        //
        // subRatio: the fixed freq multiplier for the sub oscillator octave offset
        //   (subSemitones = {-24,-12,0} depending on pSubOct, block-constant).
        // octUpRatio/octDnRatio: the fixed multiplier for the ±1 octave groups
        //   (detuneRatio = detune, also block-constant).
        // These replace fastExp calls that were redundantly evaluated every sample.
        const float subRatio = fastExp(subSemitones * (0.693147f / 12.0f));
        const float octUpRatio = fastExp((12.0f * 100.0f + detune) * (0.693147f / 1200.0f));
        const float octDnRatio = fastExp((-12.0f * 100.0f - detune) * (0.693147f / 1200.0f));

        // Bitcrusher: std::pow is block-constant for constant bitDepth parameter.
        const float crushLevels = std::max(std::pow(2.0f, std::floor(crushDepth)) - 1.0f, 1.0f);
        const float invCrushLevels = 1.0f / crushLevels;

        float peakEnv = 0.0f;

        // --- Render ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;
                ++voice.age;

                // Base frequency with glide
                float baseFreq = voice.cachedBaseFreq;
                if (voice.glideActive)
                {
                    voice.glideSourceFreq += glideCoeff * (baseFreq - voice.glideSourceFreq);
                    if (std::abs(voice.glideSourceFreq - baseFreq) < 0.1f)
                    {
                        voice.glideSourceFreq = baseFreq;
                        voice.glideActive = false;
                    }
                    baseFreq = voice.glideSourceFreq;
                }

                // Pitch mod (coupling) + pitch bend
                float freq = baseFreq * fastExp(pitchMod * (0.693147f / 12.0f)) *
                             PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

                // Filter envelope
                float fltEnvVal = voice.filterEnv.process();
                // D002/D005: LFO1 — user-controllable BreathingLFO (rate set per-block above).
                // lfo1Target: 0=Filter, 1=Mojo (B015 dynamic axis), 2=Saturation.
                // Output scaled by lfo1Depth (user control, 0–1).
                const float lfo1Raw = voice.breathingLFO.process() * lfo1Depth;
                // When target is Filter (0): add semitone sweep (×2 = ±2 semitones at depth=1).
                const float breathFilterMod = (lfo1Target == 0) ? lfo1Raw * 2.0f : 0.0f;
                // When target is Mojo (1): modulate analog amount ±0.35 at depth=1 (B015 dynamic).
                const float lfo1MojoMod = (lfo1Target == 1) ? lfo1Raw * 0.35f : 0.0f;
                // When target is Saturation (2): modulate sat drive ±0.3 at depth=1.
                const float lfo1SatMod = (lfo1Target == 2) ? (lfo1Raw * 0.3f + 0.3f * lfo1Depth) : 0.0f;

                // D001: velocity → filter brightness (via fltEnvAmt scaling)
                // D002: LFO1 filter mode adds semitone sweep; LFO2 adds ±24 semitone sweep
                float keyTrackOffset = (static_cast<float>(voice.noteNumber) - 60.0f) * fltKeyTrack;
                float cutoff = fltCutoff * fastExp((fltEnvAmt * fltEnvVal * voice.velocity * 48.0f + keyTrackOffset +
                                                    filterMod * 12.0f + breathFilterMod + lfo2SemitonesMod) *
                                                   (0.693147f / 12.0f));
                cutoff = clamp(cutoff, 20.0f, 18000.0f);

                // Compute filter coefficients ONCE per voice per sample (not 4×).
                // All 4 filters in this voice share the same cutoff/resonance.
                auto filterCoeffs = FatLadderFilter::computeCoeffs(cutoff, fltReso, 1.0f / srf, srf);
                for (int gi = 0; gi < 4; ++gi)
                {
                    voice.filters[static_cast<size_t>(gi)].applyCoeffs(filterCoeffs);
                    voice.filters[static_cast<size_t>(gi)].setDrive(fltDrive);
                }

                // D002: LFO1 Mojo target (B015 dynamic axis) — modulates analogAmount per sample.
                // lfo1MojoMod is ±0.35 at depth=1; clamped so it never exceeds [0,1].
                const float voiceAnalogAmount = clamp(analogAmount + lfo1MojoMod, 0.0f, 1.0f);

                // --- Sub oscillator ---
                // Use precomputed block-constant subRatio instead of per-sample fastExp.
                float subFreq = freq * subRatio;
                float subDriftCents = voice.subDrift.process(voiceAnalogAmount);
                subFreq *= fastExp(subDriftCents * (0.693147f / 1200.0f));
                voice.subOsc.setFrequency(subFreq);
                float subSample = voice.subOsc.processSub() * subLevel;
                // Mojo soft-clip on sub — uses LFO1-modulated voiceAnalogAmount (B015 dynamic)
                if (voiceAnalogAmount > 0.001f)
                    subSample = subSample + voiceAnalogAmount * (fastTanh(subSample) - subSample);

                // --- Group oscillators ---
                // Use precomputed block-constant octave ratios (hoisted from sample loop).
                float freqOctUp = freq * octUpRatio;
                float freqOctDn = freq * octDnRatio;
                float grpFreqs[3] = {freq, freqOctUp, freqOctDn};

                float groupSums[3] = {0.0f, 0.0f, 0.0f}; // G1-G3
                float g4L = 0.0f, g4R = 0.0f;            // G4 split stereo

                // Groups 1-3: standard summed processing
                for (int g = 0; g < 3; ++g)
                {
                    int baseIdx = g * 3;
                    float gSum = 0.0f;
                    for (int o = 0; o < 3; ++o)
                    {
                        auto idx = static_cast<size_t>(baseIdx + o);
                        float driftCents = voice.drifts[idx].process(voiceAnalogAmount);
                        float oscFreq = grpFreqs[o] * fastExp(driftCents * (0.693147f / 1200.0f));
                        voice.oscs[idx].setFrequency(oscFreq);
                        float s = voice.oscs[idx].process(morph, voice.noiseGens[idx]);
                        if (voiceAnalogAmount > 0.001f)
                            s = s + voiceAnalogAmount * (fastTanh(s) - s);
                        gSum += s;
                    }
                    gSum *= 0.3333f;
                    gSum = voice.filters[static_cast<size_t>(g)].processSample(gSum);
                    groupSums[g] = gSum;
                }

                // Group 4: wide stereo — oscs 10,11 → left, osc 12 → right
                {
                    float g4oscs[3];
                    for (int o = 0; o < 3; ++o)
                    {
                        auto idx = static_cast<size_t>(9 + o);
                        float driftCents = voice.drifts[idx].process(voiceAnalogAmount);
                        float oscFreq = grpFreqs[o] * fastExp(driftCents * (0.693147f / 1200.0f));
                        voice.oscs[idx].setFrequency(oscFreq);
                        float s = voice.oscs[idx].process(morph, voice.noiseGens[idx]);
                        if (voiceAnalogAmount > 0.001f)
                            s = s + voiceAnalogAmount * (fastTanh(s) - s);
                        g4oscs[o] = s;
                    }
                    // Filter the mono sum
                    float g4mono = (g4oscs[0] + g4oscs[1] + g4oscs[2]) * 0.3333f;
                    float g4filt = voice.filters[3].processSample(g4mono);
                    // Apply fixed stereo distribution: oscs 0,1 (2/3 weight) left, osc 2 (1/3) right
                    g4L = g4filt * g4aL * 0.667f + g4filt * g4bL * 0.333f;
                    g4R = g4filt * g4aR * 0.667f + g4filt * g4bR * 0.333f;
                }

                // --- Mix groups to stereo ---
                // G1: center
                float voiceL = groupSums[0] * g1L;
                float voiceR = groupSums[0] * g1R;

                // G2-G3 scaled by groupMix
                voiceL += groupSums[1] * g2L * groupMix;
                voiceR += groupSums[1] * g2R * groupMix;

                voiceL += groupSums[2] * g3L * groupMix;
                voiceR += groupSums[2] * g3R * groupMix;

                // G4: true wide stereo
                voiceL += g4L * groupMix;
                voiceR += g4R * groupMix;

                // Add sub (mono, center)
                voiceL += subSample * 0.707f;
                voiceR += subSample * 0.707f;

                // Amp envelope
                float envVal = voice.ampEnv.process();
                voiceL *= envVal * voice.velocity;
                voiceR *= envVal * voice.velocity;

                // Post-voice saturation — LFO1 Saturation target adds dynamic drive (B015 side-effect).
                // lfo1SatMod is non-negative when lfo1Target==2, otherwise 0.
                voice.saturation.setDrive(clamp(satDrive + lfo1SatMod, 0.0f, 1.0f));
                voiceL = voice.saturation.process(voiceL);
                voiceR = voice.saturation.process(voiceR);

                // Bitcrusher — uses precomputed levels/invLevels (avoids per-sample std::pow).
                voiceL = voice.crusher.process(voiceL, crushRate, crushLevels, invCrushLevels);
                voiceR = voice.crusher.process(voiceR, crushRate, crushLevels, invCrushLevels);

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.age = 0;
                }

                mixL += voiceL;
                mixR += voiceR;

                peakEnv = std::max(peakEnv, envVal);
            }

            // Apply level + soft limit
            float outL = fastTanh(mixL * level);
            float outR = fastTanh(mixR * level);

            outputCacheL[static_cast<size_t>(sample)] = outL;
            outputCacheR[static_cast<size_t>(sample)] = outR;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sample, outL);
                buffer.addSample(1, sample, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sample, (outL + outR) * 0.5f);
            }
        }

        // Update active voice count for UI display (read by message thread via getActiveVoiceCount)
        int cnt = 0;
        for (const auto& v : voices)
            if (v.active)
                ++cnt;
        activeVoiceCount.store(cnt, std::memory_order_relaxed);

        envelopeOutput = peakEnv;

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock(buffer.getReadPointer(0),
                                 buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr, numSamples);
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
            externalFilterMod += amount;
            break;
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

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return {params.begin(), params.end()};
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Oscillator / Mix ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_morph", 1}, "Fat Morph", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_mojo", 1}, "Fat Mojo", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_subLevel", 1}, "Fat Sub Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fat_subOct", 1}, "Fat Sub Octave", juce::StringArray{"-2 Oct", "-1 Oct", "Unison"}, 1));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_groupMix", 1}, "Fat Group Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_detune", 1}, "Fat Detune", juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 5.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_stereoWidth", 1}, "Fat Stereo Width",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        // --- Amp Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_ampAttack", 1}, "Fat Amp Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.005f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_ampDecay", 1}, "Fat Amp Decay",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.2f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_ampSustain", 1}, "Fat Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_ampRelease", 1}, "Fat Amp Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.15f));

        // --- Filter ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_fltCutoff", 1}, "Fat Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 18000.0f, 0.1f, 0.25f), 2000.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_fltReso", 1}, "Fat Filter Resonance",
                                                        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.2f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_fltDrive", 1}, "Fat Filter Drive",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_fltKeyTrack", 1}, "Fat Filter Key Track",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_fltEnvAmt", 1}, "Fat Filter Env Amount",
                                                        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_fltEnvAttack", 1}, "Fat Filter Env Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.001f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_fltEnvDecay", 1}, "Fat Filter Env Decay",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

        // --- Character ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_satDrive", 1}, "Fat Saturation Drive",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_crushDepth", 1}, "Fat Crush Depth",
                                                        juce::NormalisableRange<float>(2.0f, 16.0f, 0.1f), 16.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_crushRate", 1}, "Fat Crush Rate",
            juce::NormalisableRange<float>(500.0f, 44100.0f, 1.0f, 0.3f), 44100.0f));

        // --- Arpeggiator ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fat_arpOn", 1}, "Fat Arp On/Off", juce::StringArray{"Off", "On"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fat_arpPattern", 1}, "Fat Arp Pattern",
            juce::StringArray{"Up", "Down", "UpDown", "Random", "AsPlayed"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"fat_arpRate", 1}, "Fat Arp Rate",
                                                         juce::StringArray{"1/4", "1/8", "1/8T", "1/16", "1/32"}, 1));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fat_arpOctaves", 1}, "Fat Arp Octaves", juce::StringArray{"1", "2", "3"}, 0));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_arpGate", 1}, "Fat Arp Gate",
                                                        juce::NormalisableRange<float>(0.05f, 1.0f, 0.01f), 0.5f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_arpTempo", 1}, "Fat Arp Tempo",
                                                        juce::NormalisableRange<float>(40.0f, 300.0f, 0.1f), 120.0f));

        // --- Output ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_level", 1}, "Fat Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // --- Voice ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fat_voiceMode", 1}, "Fat Voice Mode", juce::StringArray{"Poly", "Mono", "Legato"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_glide", 1}, "Fat Glide", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fat_polyphony", 1}, "Fat Polyphony", juce::StringArray{"1", "2", "4", "6"}, 3));

        // D002/D005: LFO1 — user-controllable BreathingLFO (sine wave).
        // Target: 0=Filter (semitone modulation), 1=Mojo (B015 dynamic axis), 2=Saturation.
        // Rate floor 0.005 Hz (D005 ≤ 0.01 Hz). Default 0.07 Hz = classic slow breathing.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_lfo1Rate", 1}, "Fat LFO1 Rate",
            juce::NormalisableRange<float>(0.005f, 4.0f, 0.005f, 0.3f), 0.07f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_lfo1Depth", 1}, "Fat LFO1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f));
        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"fat_lfo1Target", 1}, "Fat LFO1 Target",
                                                         juce::StringArray{"Filter", "Mojo", "Saturation"}, 0));

        // D002: 2nd LFO — dedicated filter sweep modulator.
        // Saw LFO sweeps the ZDF ladder cutoff for slow autonomous harmonic movement.
        // Rate range 0.005–4.0 Hz (floor satisfies D005 ≤ 0.01 Hz).
        // Depth up to ±6000 Hz cutoff sweep (in log/semitone space: ±24st at max).
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fat_lfo2Rate", 1}, "Fat LFO2 Rate",
            juce::NormalisableRange<float>(0.005f, 4.0f, 0.005f, 0.3f), 0.08f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_lfo2Depth", 1}, "Fat LFO2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f));

        // XOceanus standard macros (CHARACTER, MOVEMENT, COUPLING, SPACE)
        // All default to 0.0 — existing presets are unaffected.
        //
        // M1 MOJO (CHARACTER): pushes mojo axis toward analog — more drift + soft-clip.
        //   At max: +0.5 mojo → full analog saturation on all 13 oscillators.
        // M2 GRIT (MOVEMENT): increases saturation drive (+0.6) and filter drive (+0.3).
        //   At max: satDrive + 0.6, fltDrive + 0.3 — nasty, overdriven character.
        // M3 SIZE (COUPLING): opens filter cutoff (+6000 Hz) and group mix (+0.3).
        //   At max: +6000 Hz sweep + more octave groups = massive, wide stacks.
        // M4 CRUSH (SPACE): reduces crush depth and rate for lo-fi character.
        //   At max: crushDepth - 8 bits, crushRate - 20000 Hz — crunchy, degraded.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_macroMojo", 1}, "Fat MOJO",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                     0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_macroGrit", 1}, "Fat GRIT",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                     0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_macroSize", 1}, "Fat SIZE",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                     0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fat_macroCrush", 1}, "Fat CRUSH",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    }

public:
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pMorph = apvts.getRawParameterValue("fat_morph");
        pMojo = apvts.getRawParameterValue("fat_mojo");
        pSubLevel = apvts.getRawParameterValue("fat_subLevel");
        pSubOct = apvts.getRawParameterValue("fat_subOct");
        pGroupMix = apvts.getRawParameterValue("fat_groupMix");
        pDetune = apvts.getRawParameterValue("fat_detune");
        pStereoWidth = apvts.getRawParameterValue("fat_stereoWidth");

        pAmpAttack = apvts.getRawParameterValue("fat_ampAttack");
        pAmpDecay = apvts.getRawParameterValue("fat_ampDecay");
        pAmpSustain = apvts.getRawParameterValue("fat_ampSustain");
        pAmpRelease = apvts.getRawParameterValue("fat_ampRelease");

        pFltCutoff = apvts.getRawParameterValue("fat_fltCutoff");
        pFltReso = apvts.getRawParameterValue("fat_fltReso");
        pFltDrive = apvts.getRawParameterValue("fat_fltDrive");
        pFltKeyTrack = apvts.getRawParameterValue("fat_fltKeyTrack");
        pFltEnvAmt = apvts.getRawParameterValue("fat_fltEnvAmt");
        pFltEnvAttack = apvts.getRawParameterValue("fat_fltEnvAttack");
        pFltEnvDecay = apvts.getRawParameterValue("fat_fltEnvDecay");

        pSatDrive = apvts.getRawParameterValue("fat_satDrive");
        pCrushDepth = apvts.getRawParameterValue("fat_crushDepth");
        pCrushRate = apvts.getRawParameterValue("fat_crushRate");

        pArpOn = apvts.getRawParameterValue("fat_arpOn");
        pArpPattern = apvts.getRawParameterValue("fat_arpPattern");
        pArpRate = apvts.getRawParameterValue("fat_arpRate");
        pArpOctaves = apvts.getRawParameterValue("fat_arpOctaves");
        pArpGate = apvts.getRawParameterValue("fat_arpGate");
        pArpTempo = apvts.getRawParameterValue("fat_arpTempo");

        pLevel = apvts.getRawParameterValue("fat_level");
        pVoiceMode = apvts.getRawParameterValue("fat_voiceMode");
        pGlide = apvts.getRawParameterValue("fat_glide");
        pPolyphony = apvts.getRawParameterValue("fat_polyphony");
        // XOceanus macros
        pMacroMojo = apvts.getRawParameterValue("fat_macroMojo");
        pMacroGrit = apvts.getRawParameterValue("fat_macroGrit");
        pMacroSize = apvts.getRawParameterValue("fat_macroSize");
        pMacroCrush = apvts.getRawParameterValue("fat_macroCrush");
        // D002: LFO1 (user-controllable BreathingLFO)
        pLfo1Rate = apvts.getRawParameterValue("fat_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("fat_lfo1Depth");
        pLfo1Target = apvts.getRawParameterValue("fat_lfo1Target");
        // D002: 2nd LFO
        pLfo2Rate = apvts.getRawParameterValue("fat_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("fat_lfo2Depth");
    }

private:
    SilenceGate silenceGate;

    //-- Voice management -------------------------------------------------------

    void noteOn(int noteNumber, float velocity, float glideAmt, int voiceMode, int maxPoly)
    {
        if (voiceMode == 2) // Legato
        {
            for (auto& v : voices)
            {
                if (v.active)
                {
                    if (glideAmt > 0.0f)
                    {
                        v.glideActive = true;
                        v.glideSourceFreq = v.cachedBaseFreq;
                    }
                    v.noteNumber = noteNumber;
                    v.cachedBaseFreq = midiToFreq(noteNumber);
                    v.velocity = velocity;
                    return;
                }
            }
        }

        int idx = findFreeVoice(maxPoly);
        auto& voice = voices[static_cast<size_t>(idx)];

        if (voiceMode == 1 && voice.active && glideAmt > 0.0f)
        {
            voice.glideActive = true;
            voice.glideSourceFreq = midiToFreq(voice.noteNumber);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.age = 0;
        voice.startTime = ++voiceTimestamp; // monotonic counter; smaller = older (LRU)
        voice.cachedBaseFreq = midiToFreq(noteNumber);

        // Reset oscillator/filter/FX state to prevent clicks from stolen voices
        voice.subOsc.reset();
        voice.subDrift.reset();
        for (int i = 0; i < 12; ++i)
        {
            voice.oscs[static_cast<size_t>(i)].reset();
            voice.drifts[static_cast<size_t>(i)].reset();
        }
        for (int g = 0; g < 4; ++g)
            voice.filters[static_cast<size_t>(g)].reset();
        voice.saturation.reset();
        voice.crusher.reset();

        voice.ampEnv.noteOn();
        voice.filterEnv.noteOn();
    }

    void noteOff(int noteNumber)
    {
        for (auto& v : voices)
            if (v.active && v.noteNumber == noteNumber)
            {
                if (sustainPedalDown)
                    v.sustainHeld = true; // hold until pedal released
                else
                {
                    v.ampEnv.noteOff();
                    v.filterEnv.noteOff();
                }
            }
    }

    void allVoicesOff()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.glideActive = false;
            v.sustainHeld = false;
        }
        sustainPedalDown = false;
        arp.reset();
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
    }

    int findFreeVoice(int maxPoly)
    {
        int poly = std::min(maxPoly, kMaxVoices);
        // Delegate to shared VoiceAllocator (Source/DSP/VoiceAllocator.h) — LRU strategy.
        // VoiceAllocator reads voice.active + voice.startTime; FatVoice has both fields.
        int idx = VoiceAllocator::findFreeVoice(voices, poly);
        // Kill stolen voice envelopes to prevent level bleed into new note-on.
        if (voices[static_cast<size_t>(idx)].active)
        {
            voices[static_cast<size_t>(idx)].ampEnv.kill();
            voices[static_cast<size_t>(idx)].filterEnv.kill();
        }
        return idx;
    }

    static float midiToFreq(int note) noexcept
    {
        return 440.0f * fastPow2((static_cast<float>(note) - 69.0f) * (1.0f / 12.0f));
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<FatVoice, kMaxVoices> voices;
    std::atomic<int> activeVoiceCount{0};
    bool sustainPedalDown = false;
    uint64_t voiceTimestamp = 0; // monotonic counter; assigned to voice.startTime on each note-on

    FatArpeggiator arp;

    // D006: CS-80-inspired poly aftertouch (channel pressure → mojo control)
    PolyAftertouch aftertouch;

    // D006: mod wheel — CC1 boosts Mojo (more analog drift + saturation with wheel; sensitivity 0.5)
    float modWheelValue = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;

    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers (34 params: +fat_lfo1Rate, +fat_lfo1Depth, +fat_lfo1Target)
    std::atomic<float>* pMorph = nullptr;
    std::atomic<float>* pMojo = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pSubOct = nullptr;
    std::atomic<float>* pGroupMix = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pStereoWidth = nullptr;

    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    std::atomic<float>* pFltCutoff = nullptr;
    std::atomic<float>* pFltReso = nullptr;
    std::atomic<float>* pFltDrive = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    std::atomic<float>* pFltEnvAmt = nullptr;
    std::atomic<float>* pFltEnvAttack = nullptr;
    std::atomic<float>* pFltEnvDecay = nullptr;

    std::atomic<float>* pSatDrive = nullptr;
    std::atomic<float>* pCrushDepth = nullptr;
    std::atomic<float>* pCrushRate = nullptr;

    std::atomic<float>* pArpOn = nullptr;
    std::atomic<float>* pArpPattern = nullptr;
    std::atomic<float>* pArpRate = nullptr;
    std::atomic<float>* pArpOctaves = nullptr;
    std::atomic<float>* pArpGate = nullptr;
    std::atomic<float>* pArpTempo = nullptr;

    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pPolyphony = nullptr;

    // XOceanus macros (MOJO, GRIT, SIZE, CRUSH)
    std::atomic<float>* pMacroMojo = nullptr;
    std::atomic<float>* pMacroGrit = nullptr;
    std::atomic<float>* pMacroSize = nullptr;
    std::atomic<float>* pMacroCrush = nullptr;

    // D002/D005: LFO1 — user-controllable BreathingLFO (sine).
    // Rate and depth are now user parameters; target routes to Filter/Mojo/Saturation (B015).
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;

    // D002: 2nd LFO — user-routable filter modulator.
    // Saw LFO sweeps ZDF ladder cutoff for autonomous harmonic evolution.
    // Rate floor 0.005 Hz satisfies D005 (≤ 0.01 Hz).
    double lfo2Phase = 0.0;  // [0, 1) normalized phase
    float lfo2Output = 0.0f; // cached saw output [-1, +1]
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
};

} // namespace xoceanus
