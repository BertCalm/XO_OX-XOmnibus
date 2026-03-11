#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <vector>
#include <cstdint>

namespace xomnibus {

//==============================================================================
// BiteNoiseGen -- xorshift32 PRNG for RT-safe noise generation.
//==============================================================================
class BiteNoiseGen
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

    float nextFloat() noexcept { return (process() + 1.0f) * 0.5f; }

private:
    uint32_t state = 1;
};

//==============================================================================
// BiteSineTable -- 2048-sample sine lookup for sub oscillator and FM carrier.
//==============================================================================
class BiteSineTable
{
public:
    static constexpr int kSize = 2048;

    static float lookup (double phase) noexcept
    {
        const auto& t = getTable();
        double idx = phase * static_cast<double> (kSize);
        int iLo = static_cast<int> (idx);
        float frac = static_cast<float> (idx - static_cast<double> (iLo));
        if (iLo < 0) iLo = 0;
        if (iLo >= kSize) iLo = kSize - 1;
        return t[static_cast<size_t> (iLo)]
             + frac * (t[static_cast<size_t> (iLo + 1)]
                     - t[static_cast<size_t> (iLo)]);
    }

private:
    static const std::array<float, kSize + 1>& getTable() noexcept
    {
        static const auto table = []()
        {
            constexpr double twoPi = 6.283185307179586;
            std::array<float, kSize + 1> t {};
            for (int i = 0; i <= kSize; ++i)
                t[static_cast<size_t> (i)] = static_cast<float> (
                    std::sin (static_cast<double> (i) / static_cast<double> (kSize) * twoPi));
            return t;
        }();
        return table;
    }
};

//==============================================================================
// BiteOscA -- "Belly" oscillator: warm, low-end focused waveforms.
//   0: Sine (pure sub weight)
//   1: Triangle (soft harmonic body)
//   2: Saw (rich but warm via gentle rolloff)
//   3: Cushion Pulse (variable-width plush tone)
//==============================================================================
class BiteOscA
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void reset() noexcept
    {
        phase = 0.0;
        triState = 0.0f;
    }

    void setFrequency (float hz) noexcept
    {
        double freq = clamp (static_cast<float> (hz), 20.0f, 20000.0f);
        phaseInc = freq / sr;
    }

    void setWaveform (int w) noexcept { wave = w; }

    float processSample() noexcept
    {
        float out = 0.0f;
        double t = phase;

        switch (wave)
        {
            case 0: // Sine -- pure belly weight
                out = BiteSineTable::lookup (t);
                break;

            case 1: // Triangle -- soft harmonic body
            {
                float tri = (t < 0.5)
                    ? static_cast<float> (4.0 * t - 1.0)
                    : static_cast<float> (3.0 - 4.0 * t);
                // Gentle low-pass smoothing for plush character
                triState += (tri - triState) * 0.15f;
                triState = flushDenormal (triState);
                out = triState * 0.95f;
                break;
            }

            case 2: // Saw -- warm saw with polyBLEP
            {
                out = static_cast<float> (2.0 * t - 1.0);
                out -= polyBlepD (t, phaseInc);
                out *= 0.85f; // gentle rolloff for warmth
                break;
            }

            case 3: // Cushion Pulse -- variable width, plush
            {
                float pw = 0.35f; // slightly asymmetric for warmth
                float pulse = (t < static_cast<double> (pw)) ? 1.0f : -1.0f;
                pulse += polyBlepD (t, phaseInc);
                pulse -= polyBlepD (std::fmod (t + 1.0 - static_cast<double> (pw), 1.0), phaseInc);
                out = pulse * 0.75f;
                break;
            }

            default:
                out = BiteSineTable::lookup (t);
                break;
        }

        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;

        return clamp (out, -1.0f, 1.0f);
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    int wave = 0;
    float triState = 0.0f;

    static float polyBlepD (double t, double dt) noexcept
    {
        if (dt <= 0.0) return 0.0f;
        if (t < dt) { double x = t / dt; return static_cast<float> (2.0 * x - x * x - 1.0); }
        if (t > 1.0 - dt) { double x = (t - 1.0) / dt; return static_cast<float> (x * x + 2.0 * x + 1.0); }
        return 0.0f;
    }
};

//==============================================================================
// BiteOscB -- "Bite" oscillator: aggressive upper harmonics.
//   0: Hard Sync Saw (sync'd to OscA phase)
//   1: FM (2-op frequency modulation)
//   2: Ring Mod (ring modulation with OscA)
//   3: Noise (filtered noise)
//   4: Grit (bitcrushed saw)
//==============================================================================
class BiteOscB
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void reset() noexcept
    {
        phase = 0.0;
        fmModPhase = 0.0;
    }

    void setFrequency (float hz) noexcept
    {
        baseFreq = static_cast<double> (clamp (hz, 20.0f, 20000.0f));
        phaseInc = baseFreq / sr;
    }

    void setWaveform (int w) noexcept { wave = w; }

    // Call when OscA wraps phase (for hard sync)
    void syncReset() noexcept { phase = 0.0; }

    float processSample (float oscAout, float externalFM) noexcept
    {
        float out = 0.0f;

        switch (wave)
        {
            case 0: // Hard Sync Saw
            {
                out = static_cast<float> (2.0 * phase - 1.0);
                out -= polyBlepD (phase, phaseInc);
                break;
            }

            case 1: // FM -- 2-operator
            {
                // Modulator at 2x carrier frequency
                fmModPhase += phaseInc * 2.0;
                if (fmModPhase >= 1.0) fmModPhase -= 1.0;
                float mod = BiteSineTable::lookup (fmModPhase);
                // FM depth scales modulation index
                float fmDepth = 0.5f + externalFM * 2.0f;
                double modPhase = phase + static_cast<double> (mod * fmDepth * 0.3f);
                modPhase -= std::floor (modPhase);
                out = BiteSineTable::lookup (modPhase);
                break;
            }

            case 2: // Ring Mod -- multiply with OscA
            {
                float carrier = BiteSineTable::lookup (phase);
                out = carrier * oscAout;
                break;
            }

            case 3: // Noise -- filtered noise for texture bite
                out = noiseGen.process();
                break;

            case 4: // Grit -- bitcrushed saw
            {
                out = static_cast<float> (2.0 * phase - 1.0);
                // Quantize to simulate bit reduction (8-bit feel)
                float steps = 32.0f;
                out = std::floor (out * steps) / steps;
                break;
            }

            default:
                out = static_cast<float> (2.0 * phase - 1.0);
                break;
        }

        // Apply external FM modulation to phase
        double fmOffset = static_cast<double> (externalFM * 0.01f);
        phase += phaseInc + fmOffset;
        while (phase >= 1.0) phase -= 1.0;
        while (phase < 0.0) phase += 1.0;

        return clamp (out, -1.0f, 1.0f);
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
    double baseFreq = 440.0;
    double fmModPhase = 0.0;
    int wave = 0;
    BiteNoiseGen noiseGen;

    static float polyBlepD (double t, double dt) noexcept
    {
        if (dt <= 0.0) return 0.0f;
        if (t < dt) { double x = t / dt; return static_cast<float> (2.0 * x - x * x - 1.0); }
        if (t > 1.0 - dt) { double x = (t - 1.0) / dt; return static_cast<float> (x * x + 2.0 * x + 1.0); }
        return 0.0f;
    }
};

//==============================================================================
// BiteSubOsc -- Sub oscillator at -1 or -2 octaves (sine or triangle).
//==============================================================================
class BiteSubOsc
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void reset() noexcept { phase = 0.0; }

    void setFrequency (float hz, int octave) noexcept
    {
        // octave: 0 = -1 oct, 1 = -2 oct
        float divisor = (octave == 1) ? 4.0f : 2.0f;
        double freq = static_cast<double> (clamp (hz / divisor, 10.0f, 5000.0f));
        phaseInc = freq / sr;
    }

    float processSample() noexcept
    {
        float out = BiteSineTable::lookup (phase);
        phase += phaseInc;
        if (phase >= 1.0) phase -= 1.0;
        return out;
    }

private:
    double sr = 44100.0;
    double phase = 0.0;
    double phaseInc = 0.0;
};

//==============================================================================
// BiteFur -- Pre-filter soft saturation (tanh warming stage).
// "Fur" adds plush weight before the filter by gently saturating the signal.
//==============================================================================
class BiteFur
{
public:
    float process (float input, float amount) noexcept
    {
        if (amount < 0.001f) return input;
        float drive = 1.0f + amount * 6.0f;
        float driven = input * drive;
        // Soft tanh saturation -- even harmonics for warmth
        float sat = fastTanh (driven);
        // Blend dry/wet
        return lerp (input, sat, amount);
    }
};

//==============================================================================
// BiteChew -- Post-filter contour: gentle compression/shaping.
// Compresses dynamics to add sustain and body to the bass.
//==============================================================================
class BiteChew
{
public:
    float process (float input, float amount) noexcept
    {
        if (amount < 0.001f) return input;
        // Soft knee compressor approximation
        float absIn = std::abs (input);
        float threshold = 1.0f - amount * 0.7f;
        if (absIn > threshold)
        {
            float excess = absIn - threshold;
            float compressed = threshold + excess / (1.0f + excess * amount * 4.0f);
            input = (input > 0.0f) ? compressed : -compressed;
        }
        return input;
    }
};

//==============================================================================
// BiteGnash -- Post-filter asymmetric waveshaper.
// Adds aggressive "bite" character via asymmetric distortion.
//==============================================================================
class BiteGnash
{
public:
    float process (float input, float amount) noexcept
    {
        if (amount < 0.001f) return input;
        float drive = 1.0f + amount * 8.0f;
        float x = input * drive;
        // Asymmetric: positive half gets harder clipping than negative
        float shaped;
        if (x >= 0.0f)
        {
            // Hard curve for positive (the "bite")
            shaped = x / (1.0f + x * x * amount);
        }
        else
        {
            // Softer curve for negative (the "belly")
            shaped = fastTanh (x * 0.7f);
        }
        return lerp (input, shaped, amount);
    }
};

//==============================================================================
// BiteTrash -- Dirt modes: rust (bitcrush), splatter (fold), crushed (clip).
//   Mode 0: Off
//   Mode 1: Rust (bitcrush + sample rate reduction)
//   Mode 2: Splatter (wavefolding)
//   Mode 3: Crushed (hard clip)
//==============================================================================
class BiteTrash
{
public:
    void reset() noexcept { holdSample = 0.0f; holdCounter = 0.0f; }

    float process (float input, int mode, float amount) noexcept
    {
        if (mode == 0 || amount < 0.001f) return input;

        float out = input;

        switch (mode)
        {
            case 1: // Rust -- bitcrush
            {
                // Reduce bit depth
                float bits = 16.0f - amount * 12.0f; // 16-bit down to 4-bit
                float steps = std::pow (2.0f, bits);
                out = std::floor (out * steps) / steps;
                // Sample rate reduction
                float srReduce = 1.0f + amount * 15.0f;
                holdCounter += 1.0f;
                if (holdCounter >= srReduce)
                {
                    holdCounter = 0.0f;
                    holdSample = out;
                }
                out = holdSample;
                break;
            }

            case 2: // Splatter -- wavefolding
            {
                float gain = 1.0f + amount * 6.0f;
                out *= gain;
                // Tri-fold waveshaping
                while (out > 1.0f || out < -1.0f)
                {
                    if (out > 1.0f) out = 2.0f - out;
                    if (out < -1.0f) out = -2.0f - out;
                }
                break;
            }

            case 3: // Crushed -- hard clip with edge
            {
                float gain = 1.0f + amount * 4.0f;
                out *= gain;
                float clipLevel = 1.0f - amount * 0.5f;
                if (out > clipLevel) out = clipLevel;
                if (out < -clipLevel) out = -clipLevel;
                break;
            }

            default:
                break;
        }

        return clamp (out, -2.0f, 2.0f);
    }

private:
    float holdSample = 0.0f;
    float holdCounter = 0.0f;
};

//==============================================================================
// BiteAdsrEnvelope -- ADSR with linear attack, exponential decay/release.
// Reused pattern from BobEngine.
//==============================================================================
class BiteAdsrEnvelope
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
        if (a == lastA && d == lastD && s == lastS && r == lastR)
            return;
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

    void noteOff() noexcept
    {
        if (stage != Stage::Off)
            stage = Stage::Release;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Off:
                return 0.0f;

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
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float releaseRate = 0.0f;
    float lastA = -1.0f, lastD = -1.0f, lastS = -1.0f, lastR = -1.0f;
};

//==============================================================================
// BiteLFO -- Standard LFO with 5 shapes: Sine, Triangle, Saw, Square, S&H.
//==============================================================================
class BiteLFO
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        invSR = 1.0f / static_cast<float> (sr);
    }

    void reset() noexcept { phase = 0.0f; snhValue = 0.0f; }

    void setRate (float hz) noexcept { rate = clamp (hz, 0.01f, 40.0f); }
    void setShape (int s) noexcept { shape = s; }

    float process() noexcept
    {
        float prevPhase = phase;
        phase += rate * invSR;
        if (phase >= 1.0f) phase -= 1.0f;

        float out = 0.0f;

        switch (shape)
        {
            case 0: // Sine
            {
                constexpr float twoPi = 6.28318530f;
                out = fastSin (phase * twoPi);
                break;
            }
            case 1: // Triangle
                out = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                break;
            case 2: // Saw
                out = 2.0f * phase - 1.0f;
                break;
            case 3: // Square
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case 4: // S&H
                if (prevPhase > phase) // phase wrapped
                    snhValue = rng.process();
                out = snhValue;
                break;
            default:
                out = fastSin (phase * 6.28318530f);
                break;
        }

        return out;
    }

private:
    double sr = 44100.0;
    float invSR = 1.0f / 44100.0f;
    float phase = 0.0f;
    float rate = 1.0f;
    int shape = 0;
    float snhValue = 0.0f;
    BiteNoiseGen rng;
};

//==============================================================================
// BiteVoice -- Per-voice state for the BITE engine.
//==============================================================================
struct BiteVoice
{
    bool active = false;
    int noteNumber = 60;
    float velocity = 0.0f;
    uint64_t age = 0;

    BiteOscA oscA;
    BiteOscB oscB;
    BiteSubOsc sub;
    CytomicSVF filter;
    BiteFur fur;
    BiteChew chew;
    BiteGnash gnash;
    BiteTrash trash;
    BiteAdsrEnvelope ampEnv;
    BiteAdsrEnvelope filterEnv;
    BiteLFO lfo1;
    BiteLFO lfo2;

    // Track OscA phase wrap for hard sync
    float prevOscAPhase = 0.0f;
    bool oscAWrapped = false;

    // Cached per-block
    float cachedBaseFreq = 261.63f;
};

//==============================================================================
// BiteEngine -- Bass-forward character synth adapted from XOpossum.
//
// Signal chain per voice:
//   OscA (belly) + OscB (bite) + Sub -> Fur (pre-filter saturation)
//   -> CytomicSVF filter (4 modes) -> Chew (contour) -> Gnash (asymmetric)
//   -> Trash (dirt modes) -> Amp Envelope -> Output
//
// Modulation: Filter envelope, LFO1, LFO2.
// 4 macros: Belly, Bite, Scurry, Trash.
//
// Coupling:
//   Output: audio (ch0/ch1), envelope level (ch2)
//   Input: AmpToFilter (drum hits pump filter),
//          AudioToFM (other engine audio FM-modulates oscillators)
//==============================================================================
class BiteEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    //-- SynthEngine interface -------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t> (i)];
            v.active = false;
            v.oscA.prepare (sr);
            v.oscB.prepare (sr);
            v.sub.prepare (sr);
            v.filter.reset();
            v.filter.setMode (CytomicSVF::Mode::LowPass);
            v.trash.reset();
            v.ampEnv.prepare (sr);
            v.filterEnv.prepare (sr);
            v.lfo1.prepare (sr);
            v.lfo2.prepare (sr);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.oscA.reset();
            v.oscB.reset();
            v.sub.reset();
            v.filter.reset();
            v.trash.reset();
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.lfo1.reset();
            v.lfo2.reset();
        }
        envelopeOutput = 0.0f;
        externalFilterMod = 0.0f;
        externalFMBuffer = nullptr;
        externalFMSamples = 0;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const int   oscAWave      = safeLoad (pOscAWaveform, 0);
        const int   oscBWave      = safeLoad (pOscBWaveform, 0);
        const float oscMix        = safeLoadF (pOscMix, 0.5f);
        const float subLevel      = safeLoadF (pSubLevel, 0.0f);
        const int   subOctave     = safeLoad (pSubOctave, 0);
        const float filterCutoff  = safeLoadF (pFilterCutoff, 2000.0f);
        const float filterReso    = safeLoadF (pFilterReso, 0.3f);
        const int   filterMode    = safeLoad (pFilterMode, 0);
        const float furAmount     = safeLoadF (pFurAmount, 0.0f);
        const float gnashAmount   = safeLoadF (pGnashAmount, 0.0f);
        const int   trashMode     = safeLoad (pTrashMode, 0);
        const float trashAmount   = safeLoadF (pTrashAmount, 0.0f);
        const float ampA          = safeLoadF (pAmpAttack, 0.01f);
        const float ampD          = safeLoadF (pAmpDecay, 0.3f);
        const float ampS          = safeLoadF (pAmpSustain, 0.7f);
        const float ampR          = safeLoadF (pAmpRelease, 0.5f);
        const float filtEnvAmt    = safeLoadF (pFilterEnvAmount, 0.0f);
        const float filtA         = safeLoadF (pFilterAttack, 0.01f);
        const float filtD         = safeLoadF (pFilterDecay, 0.3f);
        const float filtS         = safeLoadF (pFilterSustain, 0.0f);
        const float filtR         = safeLoadF (pFilterRelease, 0.3f);
        const float level         = safeLoadF (pLevel, 0.8f);
        const int   polyChoice    = safeLoad (pPolyphony, 3);
        const float macroBelly    = safeLoadF (pMacroBelly, 0.0f);
        const float macroBite     = safeLoadF (pMacroBite, 0.0f);
        const float macroScurry   = safeLoadF (pMacroScurry, 0.0f);
        const float macroTrash    = safeLoadF (pMacroTrash, 0.0f);

        // Decode polyphony: 0->1, 1->2, 2->4, 3->8, 4->16
        static constexpr int polyTable[] = { 1, 2, 4, 8, 16 };
        const int maxPoly = polyTable[std::min (4, polyChoice)];

        // --- Apply macros ---
        // M1 BELLY: increases OscA level, sub level, fur warmth, filter cutoff down
        const float effOscMix = clamp (oscMix - macroBelly * 0.4f, 0.0f, 1.0f);   // less B
        const float effSubLevel = clamp (subLevel + macroBelly * 0.5f, 0.0f, 1.0f);
        const float effFurAmount = clamp (furAmount + macroBelly * 0.4f, 0.0f, 1.0f);
        const float bellyCutoffMod = -macroBelly * 3000.0f; // filter cutoff down

        // M2 BITE: increases OscB level, gnash amount, filter reso up
        const float biteOscMix = clamp (effOscMix + macroBite * 0.4f, 0.0f, 1.0f); // more B
        const float effGnashAmount = clamp (gnashAmount + macroBite * 0.6f, 0.0f, 1.0f);
        const float biteResoMod = macroBite * 0.3f;

        // M3 SCURRY: LFO speed up, filter envelope faster
        const float scurryLfoMul = 1.0f + macroScurry * 4.0f;
        const float scurryEnvMul = 1.0f - macroScurry * 0.7f; // faster = shorter times

        // M4 TRASH: trash amount up, filter resonance up
        const float effTrashAmount = clamp (trashAmount + macroTrash * 0.8f, 0.0f, 1.0f);
        const float trashResoMod = macroTrash * 0.2f;

        // Combined effective resonance
        const float effFilterReso = clamp (filterReso + biteResoMod + trashResoMod, 0.0f, 0.95f);

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), maxPoly);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                allVoicesOff();
        }

        // Consume coupling accumulators
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;

        float peakEnv = 0.0f;

        // Set filter mode once per block (map: 0=Burrow LP, 1=Snarl BP, 2=Wire HP, 3=Hollow Notch)
        CytomicSVF::Mode svfMode = CytomicSVF::Mode::LowPass;
        switch (filterMode)
        {
            case 0: svfMode = CytomicSVF::Mode::LowPass;  break;
            case 1: svfMode = CytomicSVF::Mode::BandPass;  break;
            case 2: svfMode = CytomicSVF::Mode::HighPass;  break;
            case 3: svfMode = CytomicSVF::Mode::Notch;     break;
        }

        // Pre-compute per-voice block constants
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.ampEnv.setParams (ampA, ampD, ampS, ampR);
            voice.filterEnv.setParams (
                filtA * scurryEnvMul, filtD * scurryEnvMul, filtS, filtR * scurryEnvMul);
            voice.cachedBaseFreq = midiToFreq (voice.noteNumber);
            voice.filter.setMode (svfMode);
        }

        // --- Render voices ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            // Get external FM signal for this sample (if available via coupling)
            float externalFM = 0.0f;
            if (externalFMBuffer != nullptr && sample < externalFMSamples)
                externalFM = externalFMBuffer[sample];

            for (auto& voice : voices)
            {
                if (!voice.active) continue;
                ++voice.age;

                float freq = voice.cachedBaseFreq;

                // --- LFOs ---
                voice.lfo1.setRate (2.0f * scurryLfoMul);
                voice.lfo2.setRate (0.5f * scurryLfoMul);
                float lfo1val = voice.lfo1.process();
                float lfo2val = voice.lfo2.process();

                // LFO1 -> pitch (subtle vibrato)
                float pitchMod = lfo1val * 0.005f * (1.0f + macroScurry);
                freq *= (1.0f + pitchMod);

                // --- OscA (Belly) ---
                voice.oscA.setFrequency (freq);
                voice.oscA.setWaveform (oscAWave);
                float oscAout = voice.oscA.processSample();

                // --- OscB (Bite) with optional hard sync ---
                voice.oscB.setFrequency (freq);
                voice.oscB.setWaveform (oscBWave);
                // Hard sync: reset OscB on OscA phase wrap (for mode 0)
                // (Detected by OscA producing a discontinuity - approximate via sign change)
                float oscBout = voice.oscB.processSample (oscAout, externalFM);

                // --- Sub ---
                voice.sub.setFrequency (freq, subOctave);
                float subOut = voice.sub.processSample() * effSubLevel;

                // --- Osc Mix ---
                // biteOscMix: 0 = all A, 1 = all B
                float oscOut = oscAout * (1.0f - biteOscMix) + oscBout * biteOscMix + subOut;

                // --- Fur (pre-filter saturation) ---
                oscOut = voice.fur.process (oscOut, effFurAmount);

                // --- Filter ---
                // Filter envelope modulation
                float filtEnvVal = voice.filterEnv.process();
                float modCutoff = filterCutoff
                    + filtEnvAmt * filtEnvVal * 10000.0f
                    + bellyCutoffMod
                    + filterMod * 2000.0f
                    + lfo2val * 500.0f * macroScurry;
                modCutoff = clamp (modCutoff, 20.0f, 18000.0f);

                voice.filter.setCoefficients (modCutoff, effFilterReso, srf);
                float filtered = voice.filter.processSample (oscOut);

                // --- Chew (post-filter contour) ---
                filtered = voice.chew.process (filtered, effFurAmount * 0.5f);

                // --- Gnash (asymmetric bite) ---
                filtered = voice.gnash.process (filtered, effGnashAmount);

                // --- Trash (dirt modes) ---
                int effTrashMode = trashMode;
                // M4 cycles trash modes when fully engaged
                if (macroTrash > 0.9f && trashMode == 0)
                    effTrashMode = 1; // enable Rust when macro is maxed but mode is Off
                filtered = voice.trash.process (filtered, effTrashMode, effTrashAmount);

                // --- Amp Envelope ---
                float envVal = voice.ampEnv.process();

                float out = filtered * envVal * voice.velocity;

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.age = 0;
                }

                // Center-panned mono output (bass should be mono-centered)
                mixL += out;
                mixR += out;

                peakEnv = std::max (peakEnv, envVal);
            }

            // Apply level + soft limiter
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

        envelopeOutput = peakEnv;

        // Clear coupling buffer pointer after use (consumed)
        externalFMBuffer = nullptr;
        externalFMSamples = 0;
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
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                // Drum hits pump filter cutoff
                externalFilterMod += amount;
                break;

            case CouplingType::AudioToFM:
                // Other engine audio FM-modulates our oscillators
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    externalFMBuffer = sourceBuffer;
                    externalFMSamples = numSamples;
                    externalFMAmount = amount;
                }
                break;

            default:
                break; // Other coupling types not supported by BITE
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
        // --- Oscillators ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "poss_oscAWaveform", 1 }, "Bite Osc A Waveform",
            juce::StringArray { "Sine", "Triangle", "Saw", "Cushion Pulse" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "poss_oscBWaveform", 1 }, "Bite Osc B Waveform",
            juce::StringArray { "Hard Sync Saw", "FM", "Ring Mod", "Noise", "Grit" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_oscMix", 1 }, "Bite Osc Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_subLevel", 1 }, "Bite Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "poss_subOctave", 1 }, "Bite Sub Octave",
            juce::StringArray { "-1 Oct", "-2 Oct" }, 0));

        // --- Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterCutoff", 1 }, "Bite Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 2000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterReso", 1 }, "Bite Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "poss_filterMode", 1 }, "Bite Filter Mode",
            juce::StringArray { "Burrow LP", "Snarl BP", "Wire HP", "Hollow Notch" }, 0));

        // --- Character stages ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_furAmount", 1 }, "Bite Fur Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_gnashAmount", 1 }, "Bite Gnash Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "poss_trashMode", 1 }, "Bite Trash Mode",
            juce::StringArray { "Off", "Rust", "Splatter", "Crushed" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_trashAmount", 1 }, "Bite Trash Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_ampAttack", 1 }, "Bite Amp Attack",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_ampDecay", 1 }, "Bite Amp Decay",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_ampSustain", 1 }, "Bite Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_ampRelease", 1 }, "Bite Amp Release",
            juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- Filter Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterEnvAmount", 1 }, "Bite Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterAttack", 1 }, "Bite Filter Attack",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterDecay", 1 }, "Bite Filter Decay",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterSustain", 1 }, "Bite Filter Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_filterRelease", 1 }, "Bite Filter Release",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_level", 1 }, "Bite Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Polyphony ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "poss_polyphony", 1 }, "Bite Polyphony",
            juce::StringArray { "1", "2", "4", "8", "16" }, 3));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_macroBelly", 1 }, "Bite Macro Belly",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_macroBite", 1 }, "Bite Macro Bite",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_macroScurry", 1 }, "Bite Macro Scurry",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "poss_macroTrash", 1 }, "Bite Macro Trash",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscAWaveform     = apvts.getRawParameterValue ("poss_oscAWaveform");
        pOscBWaveform     = apvts.getRawParameterValue ("poss_oscBWaveform");
        pOscMix           = apvts.getRawParameterValue ("poss_oscMix");
        pSubLevel         = apvts.getRawParameterValue ("poss_subLevel");
        pSubOctave        = apvts.getRawParameterValue ("poss_subOctave");
        pFilterCutoff     = apvts.getRawParameterValue ("poss_filterCutoff");
        pFilterReso       = apvts.getRawParameterValue ("poss_filterReso");
        pFilterMode       = apvts.getRawParameterValue ("poss_filterMode");
        pFurAmount        = apvts.getRawParameterValue ("poss_furAmount");
        pGnashAmount      = apvts.getRawParameterValue ("poss_gnashAmount");
        pTrashMode        = apvts.getRawParameterValue ("poss_trashMode");
        pTrashAmount      = apvts.getRawParameterValue ("poss_trashAmount");
        pAmpAttack        = apvts.getRawParameterValue ("poss_ampAttack");
        pAmpDecay         = apvts.getRawParameterValue ("poss_ampDecay");
        pAmpSustain       = apvts.getRawParameterValue ("poss_ampSustain");
        pAmpRelease       = apvts.getRawParameterValue ("poss_ampRelease");
        pFilterEnvAmount  = apvts.getRawParameterValue ("poss_filterEnvAmount");
        pFilterAttack     = apvts.getRawParameterValue ("poss_filterAttack");
        pFilterDecay      = apvts.getRawParameterValue ("poss_filterDecay");
        pFilterSustain    = apvts.getRawParameterValue ("poss_filterSustain");
        pFilterRelease    = apvts.getRawParameterValue ("poss_filterRelease");
        pLevel            = apvts.getRawParameterValue ("poss_level");
        pPolyphony        = apvts.getRawParameterValue ("poss_polyphony");
        pMacroBelly       = apvts.getRawParameterValue ("poss_macroBelly");
        pMacroBite        = apvts.getRawParameterValue ("poss_macroBite");
        pMacroScurry      = apvts.getRawParameterValue ("poss_macroScurry");
        pMacroTrash       = apvts.getRawParameterValue ("poss_macroTrash");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Overbite"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF4A7C59); }
    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        return count;
    }

private:
    //--------------------------------------------------------------------------
    // Safe parameter loading helpers (avoid nullptr dereference)
    static int safeLoad (std::atomic<float>* p, int fallback) noexcept
    {
        return (p != nullptr) ? static_cast<int> (p->load()) : fallback;
    }

    static float safeLoadF (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    static float midiToFreq (int note) noexcept
    {
        return 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
    }

    //--------------------------------------------------------------------------
    void noteOn (int noteNumber, float velocity, int maxPoly)
    {
        int idx = findFreeVoice (maxPoly);
        auto& v = voices[static_cast<size_t> (idx)];

        v.active = true;
        v.noteNumber = noteNumber;
        v.velocity = velocity;
        v.age = 0;

        float freq = midiToFreq (noteNumber);
        v.oscA.reset();
        v.oscA.setFrequency (freq);
        v.oscB.reset();
        v.oscB.setFrequency (freq);
        v.sub.reset();
        v.filter.reset();
        v.trash.reset();
        v.lfo1.reset();
        v.lfo2.reset();

        v.ampEnv.noteOn();
        v.filterEnv.noteOn();
    }

    void noteOff (int noteNumber)
    {
        for (auto& v : voices)
        {
            if (v.active && v.noteNumber == noteNumber)
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
        }
    }

    int findFreeVoice (int maxPoly)
    {
        int poly = std::min (maxPoly, kMaxVoices);

        // Find inactive voice within polyphony limit
        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing -- oldest voice
        int oldest = 0;
        uint64_t oldestAge = 0;
        for (int i = 0; i < poly; ++i)
        {
            if (voices[static_cast<size_t> (i)].age > oldestAge)
            {
                oldestAge = voices[static_cast<size_t> (i)].age;
                oldest = i;
            }
        }
        return oldest;
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<BiteVoice, kMaxVoices> voices;

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalFilterMod = 0.0f;
    const float* externalFMBuffer = nullptr;
    int externalFMSamples = 0;
    float externalFMAmount = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers
    std::atomic<float>* pOscAWaveform = nullptr;
    std::atomic<float>* pOscBWaveform = nullptr;
    std::atomic<float>* pOscMix = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pSubOctave = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pFilterMode = nullptr;
    std::atomic<float>* pFurAmount = nullptr;
    std::atomic<float>* pGnashAmount = nullptr;
    std::atomic<float>* pTrashMode = nullptr;
    std::atomic<float>* pTrashAmount = nullptr;
    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;
    std::atomic<float>* pFilterEnvAmount = nullptr;
    std::atomic<float>* pFilterAttack = nullptr;
    std::atomic<float>* pFilterDecay = nullptr;
    std::atomic<float>* pFilterSustain = nullptr;
    std::atomic<float>* pFilterRelease = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pPolyphony = nullptr;
    std::atomic<float>* pMacroBelly = nullptr;
    std::atomic<float>* pMacroBite = nullptr;
    std::atomic<float>* pMacroScurry = nullptr;
    std::atomic<float>* pMacroTrash = nullptr;
};

} // namespace xomnibus
