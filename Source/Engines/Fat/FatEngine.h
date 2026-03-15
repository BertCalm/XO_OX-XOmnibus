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
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace xomnibus {

// FastMath functions (flushDenormal, fastTanh, fastSin, fastExp, clamp, lerp)
// are free functions in the xomnibus namespace, included via FastMath.h.

//==============================================================================
// FatNoiseGen — xorshift32 PRNG for noise oscillator.
//==============================================================================
class FatNoiseGen
{
public:
    void seed (uint32_t s) noexcept { state = s ? s : 1; }

    float next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) * (1.0f / 2147483648.0f);
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
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        phase = 0.0;
        phaseInc = 0.0;
    }

    void reset() noexcept { phase = 0.0; }

    void setFrequency (double freq) noexcept
    {
        phaseInc = clamp (static_cast<float> (freq), 20.0f, 20000.0f) / sr;
    }

    float process (float morph, FatNoiseGen& noise) noexcept
    {
        float t = static_cast<float> (phase);
        float dt = static_cast<float> (phaseInc);
        float out;

        if (morph <= 0.33f)
        {
            // Sine → Saw blend
            float sine = fastSin (t * 6.2831853f);
            float saw = 2.0f * t - 1.0f - polyBLEP (phase, phaseInc);
            out = lerp (sine, saw, morph * 3.0303f); // 1/0.33
        }
        else if (morph <= 0.66f)
        {
            // Saw → Square blend
            float saw = 2.0f * t - 1.0f - polyBLEP (phase, phaseInc);
            float sqr = (t < 0.5f ? 1.0f : -1.0f);
            sqr += polyBLEP (phase, phaseInc);
            double p2 = phase + 0.5;
            if (p2 >= 1.0) p2 -= 1.0;
            sqr -= polyBLEP (p2, phaseInc);
            out = lerp (saw, sqr, (morph - 0.33f) * 3.0303f);
        }
        else
        {
            // Square → Noise blend
            float sqr = (t < 0.5f ? 1.0f : -1.0f);
            sqr += polyBLEP (phase, phaseInc);
            double p2 = phase + 0.5;
            if (p2 >= 1.0) p2 -= 1.0;
            sqr -= polyBLEP (p2, phaseInc);
            float noiseSample = noise.next();
            float blend = (morph - 0.66f) * 2.9412f; // 1/0.34
            out = lerp (sqr, noiseSample, blend);
        }

        // Advance phase
        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;

        return out;
    }

    // Process as sub oscillator (triangle, no morph needed)
    float processSub() noexcept
    {
        float t = static_cast<float> (phase);
        float tri = 4.0f * std::abs (t - 0.5f) - 1.0f;

        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;

        return tri;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;

    static float polyBLEP (double t, double dt) noexcept
    {
        if (dt <= 0.0) return 0.0f;
        if (t < dt)
        {
            float tn = static_cast<float> (t / dt);
            return tn + tn - tn * tn - 1.0f;
        }
        if (t > 1.0 - dt)
        {
            float tn = static_cast<float> ((t - 1.0) / dt);
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
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
    }

    void seed (uint32_t s) noexcept { rng.seed (s); }

    void reset() noexcept { smoothed = 0.0f; }

    // Returns pitch drift in cents, scaled by mojoAmount
    float process (float mojoAmount) noexcept
    {
        if (mojoAmount < 0.001f) { smoothed = 0.0f; return 0.0f; }

        // Slow random walk: new target every ~100ms
        counter += 1.0f;
        float rate = 10.0f; // ~10 Hz random walk
        if (counter >= static_cast<float> (sr) / rate)
        {
            counter = 0.0f;
            target = rng.next() * 6.0f - 3.0f; // ±3 cents
        }

        // Smooth toward target
        float coeff = 0.0001f;
        smoothed += coeff * (target - smoothed);
        smoothed = flushDenormal (smoothed);

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
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float> (sr);
        s[0] = s[1] = s[2] = s[3] = 0.0f;
        coeffDirty = true;
    }

    void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    void setCutoff (float hz) noexcept
    {
        if (hz != cutoffHz) { cutoffHz = hz; coeffDirty = true; }
    }

    void setResonance (float r) noexcept
    {
        if (r != resonance) { resonance = r; coeffDirty = true; }
    }

    void setDrive (float d) noexcept { drive = d; }

    // Pre-computed coefficient bundle — avoids redundant std::tan when
    // multiple filters in a voice share the same cutoff/resonance.
    struct Coeffs { float g, G, k; };

    // Compute coefficients once per voice per sample, share across all 4 filters.
    static Coeffs computeCoeffs (float hz, float reso, float invSR, float srF) noexcept
    {
        constexpr float pi = 3.14159265f;
        float fc = clamp (hz, 20.0f, srF * 0.42f);
        float gv = std::tan (pi * fc * invSR);
        float Gv = gv / (1.0f + gv);
        float nyqRatio = fc / srF;
        float resoScale = (nyqRatio < 0.3f) ? 1.0f
            : 1.0f - 0.7f * ((nyqRatio - 0.3f) / 0.12f);
        resoScale = clamp (resoScale, 0.3f, 1.0f);
        return { gv, Gv, std::min (4.0f * reso * resoScale, 3.2f) };
    }

    // Apply pre-computed coefficients — skips std::tan.
    void applyCoeffs (const Coeffs& c) noexcept
    {
        g = c.g; G = c.G; k = c.k;
        coeffDirty = false;
    }

    float processSample (float input) noexcept
    {
        updateCoefficients();  // no-op if called after applyCoeffs()

        // Drive: pre-saturate input
        float driveGain = 1.0f + drive * 3.0f;
        float driven = fastTanh (input * driveGain);

        // Feedback from 4th pole (delayed by 1 sample)
        float fb = fastTanh (s[3]);
        float u = driven - k * fb;

        // 4 cascaded 1-pole ZDF lowpass
        for (int i = 0; i < 4; ++i)
        {
            float v = G * (u - s[i]);
            float lp = v + s[i];
            s[i] = lp + v;
            s[i] = flushDenormal (s[i]);
            lp = fastTanh (lp); // inter-stage soft-clip
            u = lp;
        }

        if (! std::isfinite (u)) u = 0.0f;
        return clamp (u, -4.0f, 4.0f);
    }

private:
    double sr = 44100.0;
    float invSR = 1.0f / 44100.0f;
    float cutoffHz = 2000.0f;
    float resonance = 0.2f;
    float drive = 0.0f;
    bool coeffDirty = true;

    float g = 0.0f, G = 0.0f, k = 0.0f;
    float s[4] = {};

    void updateCoefficients() noexcept
    {
        if (!coeffDirty) return;
        coeffDirty = false;

        constexpr float pi = 3.14159265f;
        float fc = clamp (cutoffHz, 20.0f, static_cast<float> (sr) * 0.42f);
        g = std::tan (pi * fc * invSR);
        G = g / (1.0f + g);

        // Resonance scaling: reduce near Nyquist for stability
        float nyqRatio = fc / static_cast<float> (sr);
        float resoScale = (nyqRatio < 0.3f) ? 1.0f
            : 1.0f - 0.7f * ((nyqRatio - 0.3f) / 0.12f);
        resoScale = clamp (resoScale, 0.3f, 1.0f);
        k = std::min (4.0f * resonance * resoScale, 3.2f);
    }
};

//==============================================================================
// FatEnvelope — ADSR with dirty-flag to avoid redundant std::exp calls.
//==============================================================================
class FatEnvelope
{
public:
    enum class Stage { Off, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float> (sr);
    }

    void reset() noexcept { stage = Stage::Off; level = 0.0f; }

    void setParams (float a, float d, float s, float r) noexcept
    {
        if (a == lastA && d == lastD && s == lastS && r == lastR) return;
        lastA = a; lastD = d; lastS = s; lastR = r;

        float aClamped = std::max (a, 0.001f);
        float dClamped = std::max (d, 0.001f);
        float rClamped = std::max (r, 0.001f);
        attackRate = invSR / aClamped;
        decayRate = 1.0f - std::exp (-invSR / dClamped);
        releaseRate = 1.0f - std::exp (-invSR / rClamped);
        sustain = clamp (s, 0.0f, 1.0f);
    }

    void noteOn() noexcept { stage = Stage::Attack; }
    void noteOff() noexcept { if (stage != Stage::Off) stage = Stage::Release; }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Off: return 0.0f;
            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                level -= (level - sustain) * decayRate;
                level = flushDenormal (level);
                if (level <= sustain + 0.0001f)
                {
                    level = sustain;
                    stage = Stage::Sustain;
                    if (sustain < 0.0001f) { level = 0.0f; stage = Stage::Off; }
                }
                break;
            case Stage::Sustain:
                level = sustain;
                if (sustain < 0.0001f) { level = 0.0f; stage = Stage::Off; }
                break;
            case Stage::Release:
                level -= level * releaseRate;
                level = flushDenormal (level);
                if (level < 0.0001f) { level = 0.0f; stage = Stage::Off; }
                break;
        }
        return level;
    }

    bool isActive() const noexcept { return stage != Stage::Off; }
    float currentLevel() const noexcept { return level; }

private:
    double sr = 44100.0;
    float invSR = 1.0f / 44100.0f;
    Stage stage = Stage::Off;
    float level = 0.0f;
    float sustain = 0.7f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
    float lastA = -1.0f, lastD = -1.0f, lastS = -1.0f, lastR = -1.0f;
};

//==============================================================================
// FatSaturation — Asymmetric waveshaper + DC blocker.
//==============================================================================
class FatSaturation
{
public:
    void prepare (double /*sampleRate*/) noexcept { dcPrev = 0.0f; dcOut = 0.0f; }
    void reset() noexcept { dcPrev = 0.0f; dcOut = 0.0f; }

    void setDrive (float driveAmount) noexcept
    {
        if (driveAmount != lastDrive)
        {
            lastDrive = driveAmount;
            float gain = 1.0f + driveAmount * 7.0f;
            cachedGain = gain;
            cachedOutputGain = 1.0f / std::sqrt (gain);
        }
    }

    float process (float input) noexcept
    {
        if (lastDrive < 0.001f) return input;

        float driven = input * cachedGain;

        // Asymmetric saturation: positive clips harder → even harmonics
        float wet = fastTanh (driven) * 0.7f + fastTanh (driven * 0.5f + 0.3f) * 0.3f;
        wet *= cachedOutputGain;

        // DC blocker (~5 Hz highpass)
        float dcIn = wet;
        dcOut = dcIn - dcPrev + 0.9995f * dcOut;
        dcPrev = dcIn;
        dcOut = flushDenormal (dcOut);

        return lerp (input, dcOut, lastDrive);
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
    void prepare (double sampleRate) noexcept { hostSR = sampleRate; phaseAcc = 0.0f; held = 0.0f; }
    void reset() noexcept { phaseAcc = 0.0f; held = 0.0f; rng = 0x1234abcdu; }

    // Process with precomputed levels/invLevels to avoid per-sample std::pow.
    // levels = std::pow(2.0f, std::floor(bitDepth)) - 1.0f (block-constant)
    // invLevels = 1.0f / levels
    // TPDF dither is added before quantization to break up quantization harmonics.
    float process (float input, float crushRate, float levels, float invLevels) noexcept
    {
        // Bypass if at full quality (levels == 65535 → bitDepth == 16)
        if (levels >= 65535.0f && crushRate >= static_cast<float> (hostSR) - 1.0f)
            return input;

        // Sample-and-hold
        phaseAcc += crushRate / static_cast<float> (hostSR);
        if (phaseAcc >= 1.0f)
        {
            phaseAcc -= 1.0f;

            // TPDF dither: two uniform noise samples subtracted → triangular distribution
            // amplitude = ±1 LSB, spectrally flat, decorrelates quantization error
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; // xorshift32
            const uint32_t r1 = rng;
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            const uint32_t r2 = rng;
            // Map [0, 2^31) to [-0.5, 0.5) for each, difference gives [-1, 1)
            const float dither = (static_cast<float> (r1 >> 1) - static_cast<float> (r2 >> 1))
                                 * (invLevels / static_cast<float> (0x40000000u));

            held = std::round ((input + dither) * levels) * invLevels;
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

    void prepare (double sampleRate) noexcept
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

    void noteOn (int note, float velocity) noexcept
    {
        // Add to held list if not already present
        for (int i = 0; i < numHeld; ++i)
            if (heldNotes[i] == note) return;
        if (numHeld < kMaxHeldNotes)
        {
            heldNotes[numHeld] = note;
            heldVelocities[numHeld] = velocity;
            ++numHeld;
        }
    }

    void noteOff (int note) noexcept
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

    struct ArpEvent { int note; float velocity; bool isNoteOn; };

    // Process one block: returns up to 2 events (note-off + note-on)
    // Call once per block with block size
    void processBlock (int numSamples, float bpm, int rateIndex, int patternIndex,
                       int octaves, float gate,
                       ArpEvent* events, int& numEvents) noexcept
    {
        numEvents = 0;
        if (numHeld == 0)
        {
            if (noteActive)
            {
                events[numEvents++] = { currentArpNote, 0.0f, false };
                noteActive = false;
                currentArpNote = -1;
            }
            return;
        }

        static constexpr float rateMultipliers[] = { 1.0f, 0.5f, 0.3333f, 0.25f, 0.125f };
        float rateMul = rateMultipliers[std::min (std::max (rateIndex, 0), 4)];
        int samplesPerStep = static_cast<int> ((60.0 / std::max (bpm, 40.0f)) * sr * rateMul);
        int gateSamples = static_cast<int> (static_cast<float> (samplesPerStep) * gate);

        sampleCounter += numSamples;

        // Gate off?
        if (noteActive)
        {
            gateCounter += numSamples;
            if (gateCounter >= gateSamples)
            {
                events[numEvents++] = { currentArpNote, 0.0f, false };
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
            int patIdx = getPatternIndex (patternIndex, totalNotes);

            int noteIdx = patIdx % numHeld;
            int octShift = patIdx / numHeld;

            int note = heldNotes[noteIdx] + octShift * 12;
            float vel = heldVelocities[noteIdx];

            if (noteActive && currentArpNote != note)
                events[numEvents++] = { currentArpNote, 0.0f, false };

            events[numEvents++] = { note, vel, true };
            currentArpNote = note;
            noteActive = true;

            advanceStep (patternIndex, totalNotes);
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

    int getPatternIndex (int pattern, int total) noexcept
    {
        if (total <= 0) return 0;
        switch (pattern)
        {
            case 0: return stepIndex % total;                          // Up
            case 1: return (total - 1 - stepIndex % total);            // Down
            case 2: return ascending ? (stepIndex % total)             // UpDown
                                     : (total - 1 - stepIndex % total);
            case 3: // Random
            {
                rngState ^= rngState << 13;
                rngState ^= rngState >> 17;
                rngState ^= rngState << 5;
                return static_cast<int> (rngState % static_cast<uint32_t> (total));
            }
            case 4: return stepIndex % total;                          // AsPlayed
            default: return 0;
        }
    }

    void advanceStep (int pattern, int total) noexcept
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
    uint64_t age = 0;

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

    // Envelopes
    FatEnvelope ampEnv;
    FatEnvelope filterEnv;

    // D005: Breathing LFO — autonomous filter modulation (rate floor 0.01 Hz)
    struct BreathingLFO
    {
        float phase = 0.0f;
        float sr = 44100.0f;
        void prepare (double sampleRate) noexcept { sr = static_cast<float> (sampleRate); }
        void reset() noexcept { phase = 0.0f; }
        float process (float rateHz) noexcept
        {
            float out = fastSin (phase * 6.28318530718f);
            phase += rateHz / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            return out;
        }
    } breathingLFO;

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
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFFF1493); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load (std::memory_order_relaxed); }

    //-- Lifecycle --------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        arp.prepare (sampleRate);
        aftertouch.prepare (sampleRate);  // D006: 5ms attack / 20ms release smoothing

        for (int v = 0; v < kMaxVoices; ++v)
        {
            auto& voice = voices[static_cast<size_t> (v)];
            voice.subOsc.prepare (sampleRate);
            voice.subDrift.prepare (sampleRate);
            voice.subDrift.seed (static_cast<uint32_t> (v * 7919 + 42));
            voice.subNoise.seed (static_cast<uint32_t> (v * 3001 + 7));

            for (int i = 0; i < 12; ++i)
            {
                voice.oscs[static_cast<size_t> (i)].prepare (sampleRate);
                voice.drifts[static_cast<size_t> (i)].prepare (sampleRate);
                voice.drifts[static_cast<size_t> (i)].seed (
                    static_cast<uint32_t> (v * 7919 + i * 131 + 42));
                voice.noiseGens[static_cast<size_t> (i)].seed (
                    static_cast<uint32_t> (v * 5003 + i * 271 + 99));
            }
            for (int g = 0; g < 4; ++g)
                voice.filters[static_cast<size_t> (g)].prepare (sampleRate);
            voice.ampEnv.prepare (sampleRate);
            voice.filterEnv.prepare (sampleRate);
            voice.breathingLFO.prepare (sampleRate);
            voice.saturation.prepare (sampleRate);
            voice.crusher.prepare (sampleRate);
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
                v.oscs[static_cast<size_t> (i)].reset();
                v.drifts[static_cast<size_t> (i)].reset();
            }
            for (int g = 0; g < 4; ++g) v.filters[static_cast<size_t> (g)].reset();
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.saturation.reset();
            v.crusher.reset();
        }
    }

    //-- Audio ------------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi, int numSamples) override
    {
        if (numSamples <= 0) return;

        // Snapshot parameters (block-constant)
        const float morph       = (pMorph != nullptr) ? pMorph->load() : 0.5f;
        const float mojo        = (pMojo != nullptr) ? pMojo->load() : 0.5f;
        const float subLevel    = (pSubLevel != nullptr) ? pSubLevel->load() : 0.5f;
        const int   subOct      = (pSubOct != nullptr) ? static_cast<int> (pSubOct->load()) : 1;
        const float groupMix    = (pGroupMix != nullptr) ? pGroupMix->load() : 1.0f;
        const float detune      = (pDetune != nullptr) ? pDetune->load() : 5.0f;
        const float stereoWidth = (pStereoWidth != nullptr) ? pStereoWidth->load() : 0.5f;

        const float ampA        = (pAmpAttack != nullptr) ? pAmpAttack->load() : 0.005f;
        const float ampD        = (pAmpDecay != nullptr) ? pAmpDecay->load() : 0.2f;
        const float ampS        = (pAmpSustain != nullptr) ? pAmpSustain->load() : 0.8f;
        const float ampR        = (pAmpRelease != nullptr) ? pAmpRelease->load() : 0.15f;

        const float fltCutoff   = (pFltCutoff != nullptr) ? pFltCutoff->load() : 2000.0f;
        const float fltReso     = (pFltReso != nullptr) ? pFltReso->load() : 0.2f;
        const float fltDrive    = (pFltDrive != nullptr) ? pFltDrive->load() : 0.0f;
        const float fltKeyTrack = (pFltKeyTrack != nullptr) ? pFltKeyTrack->load() : 0.5f;
        const float fltEnvAmt   = (pFltEnvAmt != nullptr) ? pFltEnvAmt->load() : 0.5f;
        const float fltEnvA     = (pFltEnvAttack != nullptr) ? pFltEnvAttack->load() : 0.001f;
        const float fltEnvD     = (pFltEnvDecay != nullptr) ? pFltEnvDecay->load() : 0.3f;

        const float satDrive    = (pSatDrive != nullptr) ? pSatDrive->load() : 0.3f;
        const float crushDepth  = (pCrushDepth != nullptr) ? pCrushDepth->load() : 16.0f;
        const float crushRate   = (pCrushRate != nullptr) ? pCrushRate->load() : 44100.0f;

        const int   arpOn       = (pArpOn != nullptr) ? static_cast<int> (pArpOn->load()) : 0;
        const int   arpPattern  = (pArpPattern != nullptr) ? static_cast<int> (pArpPattern->load()) : 0;
        const int   arpRate     = (pArpRate != nullptr) ? static_cast<int> (pArpRate->load()) : 1;
        const int   arpOctaves  = (pArpOctaves != nullptr) ? static_cast<int> (pArpOctaves->load()) + 1 : 1;
        const float arpGate     = (pArpGate != nullptr) ? pArpGate->load() : 0.5f;
        const float arpTempo    = (pArpTempo != nullptr) ? pArpTempo->load() : 120.0f;

        const float level       = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const int   voiceMode   = (pVoiceMode != nullptr) ? static_cast<int> (pVoiceMode->load()) : 0;
        const float glideAmt    = (pGlide != nullptr) ? pGlide->load() : 0.0f;
        const int   maxPoly     = (pPolyphony != nullptr)
            ? std::array<int,4>{{1,2,4,6}}[std::min (3, static_cast<int> (pPolyphony->load()))]
            : 6;

        // Sub octave: -2, -1, 0 → semitone offsets
        const float subSemitones = static_cast<float> ((subOct - 2) * 12); // index 0=-24, 1=-12, 2=0

        // Analog amount: mojo=0 → digital, mojo=1 → full analog drift
        // D006: non-const so aftertouch boost (computed post-MIDI-loop) can update it
        float analogAmount = mojo;

        // Precompute group pan gains (constant-power)
        const float g2Pan = -0.3f * stereoWidth * 2.0f;
        const float g3Pan = +0.3f * stereoWidth * 2.0f;
        const float g4PanL = -1.0f * stereoWidth * 2.0f;
        const float g4PanR = +1.0f * stereoWidth * 2.0f;

        // Pan law: L = sqrt(0.5*(1-pan)), R = sqrt(0.5*(1+pan))
        auto panGains = [] (float pan) -> std::pair<float, float>
        {
            pan = clamp (pan, -1.0f, 1.0f);
            return { std::sqrt (0.5f * (1.0f - pan)), std::sqrt (0.5f * (1.0f + pan)) };
        };

        auto [g1L, g1R] = panGains (0.0f);
        auto [g2L, g2R] = panGains (g2Pan);
        auto [g3L, g3R] = panGains (g3Pan);
        // G4: wide stereo — oscs 10,11 left, osc 12 right
        auto [g4aL, g4aR] = panGains (g4PanL);
        auto [g4bL, g4bR] = panGains (g4PanR);

        // Precompute glide coefficient
        float glideCoeff = 0.0f;
        if (glideAmt > 0.0f)
        {
            float glideTimeSec = 0.005f + glideAmt * 0.495f;
            glideCoeff = 1.0f - std::exp (-1.0f / (srf * glideTimeSec));
        }

        // --- Process MIDI (arp or direct) ---
        if (arpOn)
        {
            // Feed MIDI to arpeggiator
            for (const auto metadata : midi)
            {
                const auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                    arp.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff())
                    arp.noteOff (msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                    arp.reset();
                // D006: CC1 mod wheel → Mojo boost (classic Moog expression axis; sensitivity 0.5)
                else if (msg.isController() && msg.getControllerNumber() == 1)
                    modWheelValue = msg.getControllerValue() / 127.0f;
            }

            // Get arp events for this block
            FatArpeggiator::ArpEvent arpEvents[4];
            int numArpEvents = 0;
            arp.processBlock (numSamples, arpTempo, arpRate, arpPattern,
                              arpOctaves, arpGate, arpEvents, numArpEvents);

            for (int i = 0; i < numArpEvents; ++i)
            {
                if (arpEvents[i].isNoteOn)
                    noteOn (arpEvents[i].note, arpEvents[i].velocity,
                            glideAmt, voiceMode, maxPoly);
                else
                    noteOff (arpEvents[i].note);
            }
        }
        else
        {
            for (const auto metadata : midi)
            {
                const auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                    noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                            glideAmt, voiceMode, maxPoly);
                else if (msg.isNoteOff())
                    noteOff (msg.getNoteNumber());
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
                    aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
                // D006: CC1 mod wheel → Mojo boost (classic Moog expression axis; sensitivity 0.5)
                else if (msg.isController() && msg.getControllerNumber() == 1)
                    modWheelValue = msg.getControllerValue() / 127.0f;
            }
        }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);  // channel-mode: voice 0 holds global value

        // D006: aftertouch pushes mojo toward analog end of spectrum (sensitivity 0.3).
        // Higher pressure = more analog drift + soft-clip saturation on all oscillators.
        // This is Blessing B015 in action: the Mojo orthogonal axis becomes touch-sensitive.
        // D006: mod wheel also boosts mojo up to +0.5 at full wheel (classic Moog expression; sensitivity 0.5)
        const float effectiveMojo = clamp (mojo + atPressure * 0.3f + modWheelValue * 0.5f, 0.0f, 1.0f);
        analogAmount = effectiveMojo;  // update after MIDI loop once atPressure is known

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;

        // --- Pre-compute per-voice block constants ---
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.ampEnv.setParams (ampA, ampD, ampS, ampR);
            voice.filterEnv.setParams (fltEnvA, fltEnvD, 0.0f, 0.5f);
            voice.cachedBaseFreq = midiToFreq (voice.noteNumber);
        }

        // Block-constant precomputes — hoisted out of the sample + voice loops.
        //
        // subRatio: the fixed freq multiplier for the sub oscillator octave offset
        //   (subSemitones = {-24,-12,0} depending on pSubOct, block-constant).
        // octUpRatio/octDnRatio: the fixed multiplier for the ±1 octave groups
        //   (detuneRatio = detune, also block-constant).
        // These replace fastExp calls that were redundantly evaluated every sample.
        const float subRatio = fastExp (subSemitones * (0.693147f / 12.0f));
        const float octUpRatio = fastExp ((12.0f * 100.0f + detune) * (0.693147f / 1200.0f));
        const float octDnRatio = fastExp ((-12.0f * 100.0f - detune) * (0.693147f / 1200.0f));

        // Bitcrusher: std::pow is block-constant for constant bitDepth parameter.
        const float crushLevels = std::max (std::pow (2.0f, std::floor (crushDepth)) - 1.0f, 1.0f);
        const float invCrushLevels = 1.0f / crushLevels;

        float peakEnv = 0.0f;

        // --- Render ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;
                ++voice.age;

                // Base frequency with glide
                float baseFreq = voice.cachedBaseFreq;
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

                // Pitch mod (coupling)
                float freq = baseFreq * fastExp (pitchMod * (0.693147f / 12.0f));

                // Filter envelope
                float fltEnvVal = voice.filterEnv.process();
                // D005: breathing LFO — subtle autonomous filter modulation (0.07 Hz default)
                float breathMod = voice.breathingLFO.process (0.07f) * 2.0f;
                float keyTrackOffset = (static_cast<float> (voice.noteNumber) - 60.0f)
                                       * fltKeyTrack;
                // D001: velocity scales filter envelope depth for timbral expression
                float cutoff = fltCutoff
                    * fastExp ((fltEnvAmt * fltEnvVal * voice.velocity * 48.0f + keyTrackOffset + filterMod * 12.0f + breathMod)
                               * (0.693147f / 12.0f));
                cutoff = clamp (cutoff, 20.0f, 18000.0f);

                // Compute filter coefficients ONCE per voice per sample (not 4×).
                // All 4 filters in this voice share the same cutoff/resonance.
                auto filterCoeffs = FatLadderFilter::computeCoeffs (
                    cutoff, fltReso, 1.0f / srf, srf);
                for (int gi = 0; gi < 4; ++gi)
                {
                    voice.filters[static_cast<size_t> (gi)].applyCoeffs (filterCoeffs);
                    voice.filters[static_cast<size_t> (gi)].setDrive (fltDrive);
                }

                // --- Sub oscillator ---
                // Use precomputed block-constant subRatio instead of per-sample fastExp.
                float subFreq = freq * subRatio;
                float subDriftCents = voice.subDrift.process (analogAmount);
                subFreq *= fastExp (subDriftCents * (0.693147f / 1200.0f));
                voice.subOsc.setFrequency (subFreq);
                float subSample = voice.subOsc.processSub() * subLevel;
                // Mojo soft-clip on sub
                if (analogAmount > 0.001f)
                    subSample = subSample + analogAmount * (fastTanh (subSample) - subSample);

                // --- Group oscillators ---
                // Use precomputed block-constant octave ratios (hoisted from sample loop).
                float freqOctUp = freq * octUpRatio;
                float freqOctDn = freq * octDnRatio;
                float grpFreqs[3] = { freq, freqOctUp, freqOctDn };

                float groupSums[3] = { 0.0f, 0.0f, 0.0f }; // G1-G3
                float g4L = 0.0f, g4R = 0.0f; // G4 split stereo

                // Groups 1-3: standard summed processing
                for (int g = 0; g < 3; ++g)
                {
                    int baseIdx = g * 3;
                    float gSum = 0.0f;
                    for (int o = 0; o < 3; ++o)
                    {
                        auto idx = static_cast<size_t> (baseIdx + o);
                        float driftCents = voice.drifts[idx].process (analogAmount);
                        float oscFreq = grpFreqs[o] * fastExp (driftCents * (0.693147f / 1200.0f));
                        voice.oscs[idx].setFrequency (oscFreq);
                        float s = voice.oscs[idx].process (morph, voice.noiseGens[idx]);
                        if (analogAmount > 0.001f)
                            s = s + analogAmount * (fastTanh (s) - s);
                        gSum += s;
                    }
                    gSum *= 0.3333f;
                    gSum = voice.filters[static_cast<size_t> (g)].processSample (gSum);
                    groupSums[g] = gSum;
                }

                // Group 4: wide stereo — oscs 10,11 → left, osc 12 → right
                {
                    float g4oscs[3];
                    for (int o = 0; o < 3; ++o)
                    {
                        auto idx = static_cast<size_t> (9 + o);
                        float driftCents = voice.drifts[idx].process (analogAmount);
                        float oscFreq = grpFreqs[o] * fastExp (driftCents * (0.693147f / 1200.0f));
                        voice.oscs[idx].setFrequency (oscFreq);
                        float s = voice.oscs[idx].process (morph, voice.noiseGens[idx]);
                        if (analogAmount > 0.001f)
                            s = s + analogAmount * (fastTanh (s) - s);
                        g4oscs[o] = s;
                    }
                    // Filter the mono sum
                    float g4mono = (g4oscs[0] + g4oscs[1] + g4oscs[2]) * 0.3333f;
                    float g4filt = voice.filters[3].processSample (g4mono);
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

                // Post-voice saturation
                voice.saturation.setDrive (satDrive);
                voiceL = voice.saturation.process (voiceL);
                voiceR = voice.saturation.process (voiceR);

                // Bitcrusher — uses precomputed levels/invLevels (avoids per-sample std::pow).
                voiceL = voice.crusher.process (voiceL, crushRate, crushLevels, invCrushLevels);
                voiceR = voice.crusher.process (voiceR, crushRate, crushLevels, invCrushLevels);

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.age = 0;
                }

                mixL += voiceL;
                mixR += voiceR;

                peakEnv = std::max (peakEnv, envVal);
            }

            // Apply level + soft limit
            float outL = fastTanh (mixL * level);
            float outR = fastTanh (mixR * level);

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

        // Update active voice count for UI display (read by message thread via getActiveVoiceCount)
        int cnt = 0;
        for (const auto& v : voices) if (v.active) ++cnt;
        activeVoiceCount.store (cnt, std::memory_order_relaxed);

        envelopeOutput = peakEnv;
    }

    //-- Coupling --------------------------------------------------------------

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
        // --- Oscillator / Mix ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_morph", 1 }, "Fat Morph",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_mojo", 1 }, "Fat Mojo",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_subLevel", 1 }, "Fat Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_subOct", 1 }, "Fat Sub Octave",
            juce::StringArray { "-2 Oct", "-1 Oct", "Unison" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_groupMix", 1 }, "Fat Group Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_detune", 1 }, "Fat Detune",
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 5.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_stereoWidth", 1 }, "Fat Stereo Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_ampAttack", 1 }, "Fat Amp Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.005f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_ampDecay", 1 }, "Fat Amp Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_ampSustain", 1 }, "Fat Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_ampRelease", 1 }, "Fat Amp Release",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.15f));

        // --- Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltCutoff", 1 }, "Fat Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 18000.0f, 0.1f, 0.25f), 2000.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltReso", 1 }, "Fat Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltDrive", 1 }, "Fat Filter Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltKeyTrack", 1 }, "Fat Filter Key Track",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltEnvAmt", 1 }, "Fat Filter Env Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltEnvAttack", 1 }, "Fat Filter Env Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.001f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_fltEnvDecay", 1 }, "Fat Filter Env Decay",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.3f));

        // --- Character ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_satDrive", 1 }, "Fat Saturation Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_crushDepth", 1 }, "Fat Crush Depth",
            juce::NormalisableRange<float> (2.0f, 16.0f, 0.1f), 16.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_crushRate", 1 }, "Fat Crush Rate",
            juce::NormalisableRange<float> (500.0f, 44100.0f, 1.0f, 0.3f), 44100.0f));

        // --- Arpeggiator ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_arpOn", 1 }, "Fat Arp On/Off",
            juce::StringArray { "Off", "On" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_arpPattern", 1 }, "Fat Arp Pattern",
            juce::StringArray { "Up", "Down", "UpDown", "Random", "AsPlayed" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_arpRate", 1 }, "Fat Arp Rate",
            juce::StringArray { "1/4", "1/8", "1/8T", "1/16", "1/32" }, 1));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_arpOctaves", 1 }, "Fat Arp Octaves",
            juce::StringArray { "1", "2", "3" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_arpGate", 1 }, "Fat Arp Gate",
            juce::NormalisableRange<float> (0.05f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_arpTempo", 1 }, "Fat Arp Tempo",
            juce::NormalisableRange<float> (40.0f, 300.0f, 0.1f), 120.0f));

        // --- Output ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_level", 1 }, "Fat Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Voice ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_voiceMode", 1 }, "Fat Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "fat_glide", 1 }, "Fat Glide",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "fat_polyphony", 1 }, "Fat Polyphony",
            juce::StringArray { "1", "2", "4", "6" }, 3));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pMorph       = apvts.getRawParameterValue ("fat_morph");
        pMojo        = apvts.getRawParameterValue ("fat_mojo");
        pSubLevel    = apvts.getRawParameterValue ("fat_subLevel");
        pSubOct      = apvts.getRawParameterValue ("fat_subOct");
        pGroupMix    = apvts.getRawParameterValue ("fat_groupMix");
        pDetune      = apvts.getRawParameterValue ("fat_detune");
        pStereoWidth = apvts.getRawParameterValue ("fat_stereoWidth");

        pAmpAttack   = apvts.getRawParameterValue ("fat_ampAttack");
        pAmpDecay    = apvts.getRawParameterValue ("fat_ampDecay");
        pAmpSustain  = apvts.getRawParameterValue ("fat_ampSustain");
        pAmpRelease  = apvts.getRawParameterValue ("fat_ampRelease");

        pFltCutoff   = apvts.getRawParameterValue ("fat_fltCutoff");
        pFltReso     = apvts.getRawParameterValue ("fat_fltReso");
        pFltDrive    = apvts.getRawParameterValue ("fat_fltDrive");
        pFltKeyTrack = apvts.getRawParameterValue ("fat_fltKeyTrack");
        pFltEnvAmt   = apvts.getRawParameterValue ("fat_fltEnvAmt");
        pFltEnvAttack = apvts.getRawParameterValue ("fat_fltEnvAttack");
        pFltEnvDecay = apvts.getRawParameterValue ("fat_fltEnvDecay");

        pSatDrive    = apvts.getRawParameterValue ("fat_satDrive");
        pCrushDepth  = apvts.getRawParameterValue ("fat_crushDepth");
        pCrushRate   = apvts.getRawParameterValue ("fat_crushRate");

        pArpOn       = apvts.getRawParameterValue ("fat_arpOn");
        pArpPattern  = apvts.getRawParameterValue ("fat_arpPattern");
        pArpRate     = apvts.getRawParameterValue ("fat_arpRate");
        pArpOctaves  = apvts.getRawParameterValue ("fat_arpOctaves");
        pArpGate     = apvts.getRawParameterValue ("fat_arpGate");
        pArpTempo    = apvts.getRawParameterValue ("fat_arpTempo");

        pLevel       = apvts.getRawParameterValue ("fat_level");
        pVoiceMode   = apvts.getRawParameterValue ("fat_voiceMode");
        pGlide       = apvts.getRawParameterValue ("fat_glide");
        pPolyphony   = apvts.getRawParameterValue ("fat_polyphony");
    }

private:
    //-- Voice management -------------------------------------------------------

    void noteOn (int noteNumber, float velocity, float glideAmt, int voiceMode, int maxPoly)
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
                    v.cachedBaseFreq = midiToFreq (noteNumber);
                    v.velocity = velocity;
                    return;
                }
            }
        }

        int idx = findFreeVoice (maxPoly);
        auto& voice = voices[static_cast<size_t> (idx)];

        if (voiceMode == 1 && voice.active && glideAmt > 0.0f)
        {
            voice.glideActive = true;
            voice.glideSourceFreq = midiToFreq (voice.noteNumber);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.age = 0;
        voice.cachedBaseFreq = midiToFreq (noteNumber);

        // Reset oscillator/filter/FX state to prevent clicks from stolen voices
        voice.subOsc.reset();
        voice.subDrift.reset();
        for (int i = 0; i < 12; ++i)
        {
            voice.oscs[static_cast<size_t> (i)].reset();
            voice.drifts[static_cast<size_t> (i)].reset();
        }
        for (int g = 0; g < 4; ++g)
            voice.filters[static_cast<size_t> (g)].reset();
        voice.saturation.reset();
        voice.crusher.reset();

        voice.ampEnv.noteOn();
        voice.filterEnv.noteOn();
    }

    void noteOff (int noteNumber)
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

    int findFreeVoice (int maxPoly)
    {
        int poly = std::min (maxPoly, kMaxVoices);
        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        int oldest = 0;
        uint64_t oldestAge = voices[0].age;
        for (int i = 1; i < poly; ++i)
        {
            if (voices[static_cast<size_t> (i)].age > oldestAge)
            {
                oldestAge = voices[static_cast<size_t> (i)].age;
                oldest = i;
            }
        }
        voices[static_cast<size_t> (oldest)].ampEnv.reset();
        voices[static_cast<size_t> (oldest)].filterEnv.reset();
        return oldest;
    }

    static float midiToFreq (int note) noexcept
    {
        return 440.0f * fastPow2 ((static_cast<float> (note) - 69.0f) * (1.0f / 12.0f));
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<FatVoice, kMaxVoices> voices;
    std::atomic<int> activeVoiceCount { 0 };
    bool sustainPedalDown = false;

    FatArpeggiator arp;

    // D006: CS-80-inspired poly aftertouch (channel pressure → mojo control)
    PolyAftertouch aftertouch;

    // D006: mod wheel — CC1 boosts Mojo (more analog drift + saturation with wheel; sensitivity 0.5)
    float modWheelValue = 0.0f;

    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;

    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers (31 params)
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
};

} // namespace xomnibus
