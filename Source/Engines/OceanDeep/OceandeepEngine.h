#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>
namespace xomnibus {

//==============================================================================
//
//  OCEANDEEP ENGINE — Abyssal Bass Synthesizer
//  XOmnibus Engine Module | Accent: Trench Violet #2D0A4E
//
//  Creature: Anglerfish / Gulper Eel — bottom-of-the-water-column predator
//  Habitat:  THE HADAL ZONE — 6,000–11,000 m depth, perpetual darkness,
//            crushing pressure, bioluminescent flickers in the void.
//
//  XOceanDeep lives at the absolute floor of the XO_OX water column. It is the
//  engine of pressure and darkness: pure Oscar polarity, sub-bass presence that
//  displaces air before sound reaches your ears. Where OPENSKY launches upward,
//  OCEANDEEP pulls everything toward the trench.
//
//  Architecture:
//    1. Sub oscillator stack — 3 sine oscillators (fundamental, -1 oct, -2 oct)
//    2. Hydrostatic compressor — peak-sensing gain reduction (pressure depth)
//    3. Waveguide body — comb filter (shipwreck hull / underwater cave resonance)
//    4. Bioluminescent exciter — slowly modulated bandpass noise bursts (alien life)
//    5. Darkness filter — 2-pole Butterworth LP (50-800 Hz, signature abyssal tone)
//    6. Amp ADSR envelope
//    7. Abyssal reverb tail (optional, controlled by deep_reverbMix)
//
//  4 Macros:
//    deep_macroPressure  — hydrostatic compression depth + sub level
//    deep_macroCreature  — bioluminescent exciter level + rate
//    deep_macroWreck     — waveguide body mix + character
//    deep_macroAbyss     — darkness filter sweep (close filter) + reverb tail
//
//  ~45 deep_ parameters — doctrine compliant D001–D006.
//
//  Parameter prefix: deep_   Engine ID: "OceanDeep"
//
//  SilenceGate: 500 ms hold (long bass tails).
//
//==============================================================================

// ---------------------------------------------------------------------------
// Deep sine oscillator — single voice phase accumulator
// Used for fundamental, sub-octave (-1), and sub-sub-octave (-2).
// ---------------------------------------------------------------------------
struct DeepSineOsc {
    float phase = 0.f;
    float sr    = 44100.f;

    void prepare(double s) { sr = (float)s; }
    void reset()           { phase = 0.f; }

    // freq in Hz. Returns one sample.
    float tick(float freq) {
        phase += freq / sr;
        if (phase >= 1.f) phase -= 1.f;
        return fastSin(phase * 6.2831853f);
    }
};

// ---------------------------------------------------------------------------
// Hydrostatic compressor — peak-sensing gain reduction
// Models the weight of the water column pressing down on the signal.
// gain = 1 / (1 + pressureAmt * peakDetector)
// peakDetector is a one-pole envelope follower.
// ---------------------------------------------------------------------------
struct DeepHydroCompressor {
    float peak = 0.f;

    void reset() { peak = 0.f; }

    // attackCoeff / releaseCoeff: one-pole smoother coefficients
    float process(float in, float pressureAmt, float attackCoeff, float releaseCoeff) {
        float absSample = std::fabs(in);
        // Peak follower: fast attack, slow release
        if (absSample > peak)
            peak = peak + attackCoeff  * (absSample - peak);
        else
            peak = peak + releaseCoeff * (absSample - peak);
        peak = flushDenormal(peak);

        float gain = 1.0f / (1.0f + pressureAmt * peak);
        return in * gain;
    }
};

// ---------------------------------------------------------------------------
// Waveguide body — comb filter tuned to fundamental frequency
// Simulates a shipwreck hull or underwater cave resonance.
// y[n] = input[n] + bodyFeedback * y[n - delaySamples]
//
// bodyChar selects character mode:
//   0 = open water (light feedback, standard comb period)
//   1 = cave        (higher feedback, slightly detuned comb = cave reflections)
//   2 = wreck       (high feedback, short comb + allpass diffusion = hull modes)
//
// Buffer is pre-allocated at prepare() to max delay (1 Hz fundamental → 44100 samples).
// No allocation in renderBlock().
// ---------------------------------------------------------------------------
struct DeepWaveguideBody {
    static constexpr int kMaxDelay = 48001; // covers 1 Hz fundamental at up to 48 kHz

    float buf[kMaxDelay] = {};
    int   writePos  = 0;
    float sr        = 44100.f;

    void prepare(double s) {
        sr = (float)s;
        reset();
    }

    void reset() {
        std::fill(std::begin(buf), std::end(buf), 0.f);
        writePos = 0;
    }

    // freq: fundamental Hz | feedback: 0-0.9 | bodyChar: 0/1/2
    float tick(float in, float freq, float feedback, int bodyChar) {
        // Choose comb tuning factor based on character
        float tuneOffset = 0.f;
        if (bodyChar == 1) tuneOffset =  0.012f; // cave — slightly flat resonance
        if (bodyChar == 2) tuneOffset = -0.025f; // wreck — hull harmonic shift

        float f = clamp(freq * (1.f + tuneOffset), 20.f, sr * 0.48f);
        int delaySamples = (int)(sr / f);
        if (delaySamples < 2)   delaySamples = 2;
        if (delaySamples >= kMaxDelay) delaySamples = kMaxDelay - 1;

        // Read from circular buffer
        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += kMaxDelay;

        float delayed = buf[readPos];
        float out = in + feedback * delayed;
        out = flushDenormal(out);

        // For wreck mode: light allpass diffusion inside the comb
        if (bodyChar == 2) {
            float g = 0.3f;
            out = (out - g * delayed) / (1.f - g * out);
            out = flushDenormal(out);
        }

        buf[writePos] = out;
        writePos = (writePos + 1 >= kMaxDelay) ? 0 : writePos + 1;

        return out;
    }
};

// ---------------------------------------------------------------------------
// Bioluminescent exciter — bandpass-filtered noise triggered at irregular
// intervals by an LFO. Models deep-sea creature bioluminescence: intermittent
// alien light pulses in the darkness.
//
// The LFO phase crossing zero from below triggers a burst envelope.
// Bandpass center frequency controlled by deep_bioBrightness (200-4000 Hz).
// ---------------------------------------------------------------------------
struct DeepBioExciter {
    float lfoPhase     = 0.f;   // creature LFO accumulator
    float burstEnv     = 0.f;   // burst envelope level
    float bpState1     = 0.f;   // bandpass filter state (z1)
    float bpState2     = 0.f;   // bandpass filter state (z2)
    float noiseState   = 0.f;   // pink-ish noise filter state
    bool  lfoWasHigh   = false; // edge detect for trigger

    // Reproducible noise — lcg updated per tick
    uint32_t rng = 0x1234ABCD;

    void reset() {
        lfoPhase = burstEnv = bpState1 = bpState2 = noiseState = 0.f;
        lfoWasHigh = false;
        rng = 0x1234ABCD;
    }

    // Advance the noise source — white noise via LCG
    float nextNoise() {
        rng = rng * 1664525u + 1013904223u;
        return (float)(int32_t)rng * (1.f / 2147483648.f); // [-1, 1]
    }

    // lfoRate: 0.01-0.5 Hz | bioBrightness: 200-4000 Hz | sr: sample rate
    // Returns one sample of bio excitation.
    float tick(float lfoRate, float bioBrightness, float sr) {
        // Advance LFO (triangle shape for smooth creature breathing)
        lfoPhase += lfoRate / sr;
        if (lfoPhase >= 1.f) lfoPhase -= 1.f;
        float lfoVal = (lfoPhase < 0.5f)
                     ? (lfoPhase * 4.f - 1.f)    // rising: -1 → +1
                     : (3.f - lfoPhase * 4.f);   // falling: +1 → -1

        // Trigger burst envelope on positive zero-crossing
        bool lfoHigh = (lfoVal > 0.f);
        if (lfoHigh && !lfoWasHigh) {
            burstEnv = 1.0f; // trigger!
        }
        lfoWasHigh = lfoHigh;

        // Burst envelope — fast attack (already triggered), exponential decay
        float burstDecay = fastExp(-8.0f / sr); // ~125 ms decay at 44100
        burstEnv *= burstDecay;
        burstEnv = flushDenormal(burstEnv);

        // Generate noise sample
        float noise = nextNoise();

        // One-pole "pink-ish" pre-filter on noise (softens harshness)
        noiseState = noiseState * 0.98f + noise * 0.02f;
        noiseState = flushDenormal(noiseState);
        noise = noise - noiseState; // high-shelf tilt

        // 2-pole bandpass filter (State Variable Filter, topology: HP → LP)
        // Using simple matched-Z 2-pole BPF via sequential one-poles
        float fc    = clamp(bioBrightness, 50.f, sr * 0.45f);
        float coeff = fastExp(-6.2831853f * fc / sr);

        bpState1 = bpState1 * coeff + noise * (1.f - coeff);
        bpState2 = bpState2 * coeff + bpState1 * (1.f - coeff);
        bpState1 = flushDenormal(bpState1);
        bpState2 = flushDenormal(bpState2);
        float bp  = bpState1 - bpState2; // approximate bandpass

        return bp * burstEnv;
    }
};

// ---------------------------------------------------------------------------
// Darkness filter — 2-pole Butterworth lowpass
// The signature abyssal tone: frequency response that dies below 800 Hz,
// rolls off everything bright, leaving only pressure and darkness behind.
//
// Butterworth design via bilinear transform (Zölzer cookbook).
// Cutoff range: 50–800 Hz.
// ---------------------------------------------------------------------------
struct DeepDarknessFilter {
    float x1=0.f, x2=0.f;
    float y1=0.f, y2=0.f;
    float b0=1.f, b1=0.f, b2=0.f;
    float a1=0.f, a2=0.f;
    float lastFc=-1.f, lastQ=-1.f;

    void reset() {
        x1=x2=y1=y2=0.f;
        lastFc=lastQ=-1.f;
        b0=1.f; b1=b2=a1=a2=0.f;
    }

    // Recompute Butterworth 2-pole LP coefficients when fc or Q changes.
    // Uses prewarped bilinear transform: omega = 2*sr*tan(pi*fc/sr)
    void computeCoeffs(float fc, float Q, float sr) {
        if (fc == lastFc && Q == lastQ) return;
        lastFc = fc; lastQ = Q;

        fc = clamp(fc, 20.f, sr * 0.45f);
        Q  = clamp(Q,  0.5f, 20.f);

        float w0   = 6.2831853f * fc / sr;
        float cosW = fastCos(w0);
        float sinW = fastSin(w0);
        float alpha = sinW / (2.f * Q);

        float b0r = (1.f - cosW) * 0.5f;
        float b1r = 1.f - cosW;
        float b2r = (1.f - cosW) * 0.5f;
        float a0r = 1.f + alpha;
        float a1r = -2.f * cosW;
        float a2r = 1.f - alpha;

        float inv = 1.f / a0r;
        b0 = b0r * inv;
        b1 = b1r * inv;
        b2 = b2r * inv;
        a1 = a1r * inv;
        a2 = a2r * inv;
    }

    float process(float in, float fc, float Q, float sr) {
        computeCoeffs(fc, Q, sr);
        float out = b0*in + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        out = flushDenormal(out);
        x2=x1; x1=in;
        y2=y1; y1=out;
        return out;
    }
};

// ---------------------------------------------------------------------------
// Abyssal reverb — short dense Schroeder reverb for underwater cave tail
// Less diffuse than sky reverb: small room, dense reflections, dark.
// ---------------------------------------------------------------------------
struct DeepAbyssalReverb {
    static constexpr int kCombs = 4;
    // Short comb lengths for a tight dark room feel (cave / hull)
    static constexpr int kCombLensL[kCombs] = { 557, 601, 641, 683 };
    static constexpr int kCombLensR[kCombs] = { 571, 617, 659, 701 };
    static constexpr int kAP1Len = 113, kAP2Len = 223;

    float combBufL[kCombs][701] = {};
    float combBufR[kCombs][701] = {};
    int   combPosL[kCombs]      = {};
    int   combPosR[kCombs]      = {};
    float combStateL[kCombs]    = {};
    float combStateR[kCombs]    = {};

    float ap1BufL[kAP1Len] = {}, ap1BufR[kAP1Len] = {};
    float ap2BufL[kAP2Len] = {}, ap2BufR[kAP2Len] = {};
    int ap1PosL=0, ap1PosR=0, ap2PosL=0, ap2PosR=0;

    void reset() {
        for (int i = 0; i < kCombs; ++i) {
            std::fill(combBufL[i], combBufL[i] + kCombLensL[i], 0.f); combStateL[i]=0.f; combPosL[i]=0;
            std::fill(combBufR[i], combBufR[i] + kCombLensR[i], 0.f); combStateR[i]=0.f; combPosR[i]=0;
        }
        std::fill(ap1BufL, ap1BufL+kAP1Len, 0.f);
        std::fill(ap1BufR, ap1BufR+kAP1Len, 0.f);
        std::fill(ap2BufL, ap2BufL+kAP2Len, 0.f);
        std::fill(ap2BufR, ap2BufR+kAP2Len, 0.f);
        ap1PosL=ap1PosR=ap2PosL=ap2PosR=0;
    }

    void prepare(double /*sr*/) { reset(); }

    // space: 0-1 scales feedback 0.60→0.85 (dark short room)
    // mix:   wet mix 0-1
    void process(float& inL, float& inR, float space, float mix) {
        float fb = 0.60f + space * 0.25f;
        float dryL = inL, dryR = inR;

        auto runComb = [&](float* buf, int& pos, float& state,
                           int len, float input) -> float {
            float rd = buf[pos];
            // Very dark damping — high-frequency absorption, like deep sea
            state = flushDenormal(rd * 0.60f + state * 0.40f);
            float wr = input + fb * state;
            wr = flushDenormal(wr);
            buf[pos] = wr;
            pos = (pos + 1 >= len) ? 0 : pos + 1;
            return rd;
        };

        auto runAP = [&](float* buf, int& pos, int len, float input) -> float {
            float g = 0.5f;
            float rd = buf[pos];
            float wr = input + g * rd;
            wr = flushDenormal(wr);
            buf[pos] = wr;
            pos = (pos + 1 >= len) ? 0 : pos + 1;
            return rd - g * wr;
        };

        float wL = 0.f, wR = 0.f;
        for (int i = 0; i < kCombs; ++i) {
            wL += runComb(combBufL[i], combPosL[i], combStateL[i], kCombLensL[i], inL);
            wR += runComb(combBufR[i], combPosR[i], combStateR[i], kCombLensR[i], inR);
        }
        wL *= 0.25f; wR *= 0.25f;

        wL = runAP(ap1BufL, ap1PosL, kAP1Len, wL);
        wL = runAP(ap2BufL, ap2PosL, kAP2Len, wL);
        wR = runAP(ap1BufR, ap1PosR, kAP1Len, wR);
        wR = runAP(ap2BufR, ap2PosR, kAP2Len, wR);

        inL = dryL * (1.f - mix) + wL * mix;
        inR = dryR * (1.f - mix) + wR * mix;
    }
};

//==============================================================================
// OceandeepEngine — the full SynthEngine implementation
//==============================================================================
class OceandeepEngine : public SynthEngine {
public:
    OceandeepEngine()  = default;
    ~OceandeepEngine() = default;

    //--------------------------------------------------------------------------
    // Static parameter registration (called by XOmnibusProcessor)
    //--------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P  = juce::ParameterID;
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;
        auto nr  = [](float lo, float hi, float step=0.f) {
            return juce::NormalisableRange<float>(lo, hi, step);
        };

        // --- Sub oscillator stack ---
        params.push_back(std::make_unique<PF>(P("deep_subLevel",1),    "Sub Level",
            nr(0.f, 1.f), 0.7f));
        params.push_back(std::make_unique<PF>(P("deep_subOctMix",1),   "Sub Oct Mix",
            nr(0.f, 1.f), 0.4f));

        // --- Darkness filter ---
        params.push_back(std::make_unique<PF>(P("deep_filterCutoff",1),"Filter Cutoff",
            nr(50.f, 800.f), 400.f));
        params.push_back(std::make_unique<PF>(P("deep_filterRes",1),   "Filter Resonance",
            nr(0.f, 0.95f), 0.5f));
        params.push_back(std::make_unique<PF>(P("deep_velCutoffAmt",1),"Vel Cutoff Amount",
            nr(0.f, 1.f), 0.4f));

        // --- Waveguide body ---
        params.push_back(std::make_unique<PI>(P("deep_bodyChar",1),    "Body Character",
            0, 2, 0));
        params.push_back(std::make_unique<PF>(P("deep_bodyFeedback",1),"Body Feedback",
            nr(0.f, 0.9f), 0.45f));
        params.push_back(std::make_unique<PF>(P("deep_bodyMix",1),     "Body Mix",
            nr(0.f, 1.f), 0.3f));

        // --- Bioluminescent exciter ---
        params.push_back(std::make_unique<PF>(P("deep_bioRate",1),     "Bio Rate",
            nr(0.01f, 0.5f), 0.08f));
        params.push_back(std::make_unique<PF>(P("deep_bioMix",1),      "Bio Mix",
            nr(0.f, 1.f), 0.15f));
        params.push_back(std::make_unique<PF>(P("deep_bioBrightness",1),"Bio Brightness",
            nr(200.f, 4000.f), 800.f));

        // --- Hydrostatic compressor ---
        params.push_back(std::make_unique<PF>(P("deep_pressureAmt",1), "Pressure Amount",
            nr(0.f, 1.f), 0.6f));

        // --- Amp envelope ---
        params.push_back(std::make_unique<PF>(P("deep_ampAtk",1),      "Amp Attack",
            nr(0.001f, 0.5f), 0.01f));
        params.push_back(std::make_unique<PF>(P("deep_ampDec",1),      "Amp Decay",
            nr(0.1f,  5.0f),  0.5f));
        params.push_back(std::make_unique<PF>(P("deep_ampSus",1),      "Amp Sustain",
            nr(0.f, 1.f), 0.8f));
        params.push_back(std::make_unique<PF>(P("deep_ampRel",1),      "Amp Release",
            nr(0.2f, 8.0f), 1.5f));

        // --- LFO 1: creature modulation ---
        params.push_back(std::make_unique<PF>(P("deep_lfo1Rate",1),    "LFO1 Rate",
            nr(0.01f, 2.0f), 0.15f));
        params.push_back(std::make_unique<PF>(P("deep_lfo1Depth",1),   "LFO1 Depth",
            nr(0.f, 1.f), 0.3f));

        // --- LFO 2: pressure wobble ---
        params.push_back(std::make_unique<PF>(P("deep_lfo2Rate",1),    "LFO2 Rate",
            nr(0.01f, 0.5f), 0.05f));
        params.push_back(std::make_unique<PF>(P("deep_lfo2Depth",1),   "LFO2 Depth",
            nr(0.f, 1.f), 0.2f));

        // --- Reverb ---
        params.push_back(std::make_unique<PF>(P("deep_reverbMix",1),   "Reverb Mix",
            nr(0.f, 1.f), 0.35f));

        // --- 4 Macros ---
        params.push_back(std::make_unique<PF>(P("deep_macroPressure",1),"PRESSURE",
            nr(0.f, 1.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("deep_macroCreature",1),"CREATURE",
            nr(0.f, 1.f), 0.3f));
        params.push_back(std::make_unique<PF>(P("deep_macroWreck",1),   "WRECK",
            nr(0.f, 1.f), 0.4f));
        params.push_back(std::make_unique<PF>(P("deep_macroAbyss",1),   "ABYSS",
            nr(0.f, 1.f), 0.5f));
    }

    //--------------------------------------------------------------------------
    // SynthEngine interface
    //--------------------------------------------------------------------------
    juce::String   getEngineId()     const override { return "OceanDeep"; }
    juce::Colour   getAccentColour() const override { return juce::Colour(0xff2D0A4E); }
    int            getMaxVoices()    const override { return 1; } // monophonic bass engine

    int getActiveVoiceCount() const override {
        return (noteIsOn || ampEnvStage != EnvStage::Idle) ? 1 : 0;
    }

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = (float)sampleRate;

        oscFund.prepare(sampleRate);
        oscSub1.prepare(sampleRate);
        oscSub2.prepare(sampleRate);
        compressor.reset();
        body.prepare(sampleRate);
        bioExciter.reset();
        darknessFilter.reset();
        reverb.prepare(sampleRate);

        ampEnvStage = EnvStage::Idle;
        ampEnvLevel = 0.f;
        noteIsOn    = false;
        currentNote = 60;
        currentVel  = 1.f;
        lfo1Phase   = 0.f;
        lfo2Phase   = 0.f;
        modWheelVal = 0.f;
        aftertouchVal= 0.f;

        prepareSilenceGate(sampleRate, maxBlockSize, 500.f);
        (void)maxBlockSize;
    }

    void releaseResources() override {}

    void reset() override
    {
        oscFund.reset();
        oscSub1.reset();
        oscSub2.reset();
        compressor.reset();
        body.reset();
        bioExciter.reset();
        darknessFilter.reset();
        reverb.reset();

        ampEnvStage = EnvStage::Idle;
        ampEnvLevel = 0.f;
        noteIsOn    = false;
        lfo1Phase   = 0.f;
        lfo2Phase   = 0.f;
    }

    //--------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return { params.begin(), params.end() };
    }

    //--------------------------------------------------------------------------
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        p_subLevel       = apvts.getRawParameterValue("deep_subLevel");
        p_subOctMix      = apvts.getRawParameterValue("deep_subOctMix");
        p_filterCutoff   = apvts.getRawParameterValue("deep_filterCutoff");
        p_filterRes      = apvts.getRawParameterValue("deep_filterRes");
        p_velCutoffAmt   = apvts.getRawParameterValue("deep_velCutoffAmt");
        p_bodyChar       = apvts.getRawParameterValue("deep_bodyChar");
        p_bodyFeedback   = apvts.getRawParameterValue("deep_bodyFeedback");
        p_bodyMix        = apvts.getRawParameterValue("deep_bodyMix");
        p_bioRate        = apvts.getRawParameterValue("deep_bioRate");
        p_bioMix         = apvts.getRawParameterValue("deep_bioMix");
        p_bioBrightness  = apvts.getRawParameterValue("deep_bioBrightness");
        p_pressureAmt    = apvts.getRawParameterValue("deep_pressureAmt");
        p_ampAtk         = apvts.getRawParameterValue("deep_ampAtk");
        p_ampDec         = apvts.getRawParameterValue("deep_ampDec");
        p_ampSus         = apvts.getRawParameterValue("deep_ampSus");
        p_ampRel         = apvts.getRawParameterValue("deep_ampRel");
        p_lfo1Rate       = apvts.getRawParameterValue("deep_lfo1Rate");
        p_lfo1Depth      = apvts.getRawParameterValue("deep_lfo1Depth");
        p_lfo2Rate       = apvts.getRawParameterValue("deep_lfo2Rate");
        p_lfo2Depth      = apvts.getRawParameterValue("deep_lfo2Depth");
        p_reverbMix      = apvts.getRawParameterValue("deep_reverbMix");
        p_macroPressure  = apvts.getRawParameterValue("deep_macroPressure");
        p_macroCreature  = apvts.getRawParameterValue("deep_macroCreature");
        p_macroWreck     = apvts.getRawParameterValue("deep_macroWreck");
        p_macroAbyss     = apvts.getRawParameterValue("deep_macroAbyss");
    }

    //--------------------------------------------------------------------------
    void applyCouplingInput(CouplingType type, float amount,
                            const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        // OCEANDEEP accepts coupling as modulation on filter cutoff (AmpToFilter),
        // or pitch offset (AmpToPitch / PitchToPitch).
        if (type == CouplingType::AmpToFilter)
            couplingFilterMod += amount * 200.f; // ±200 Hz range
        else if (type == CouplingType::AmpToPitch || type == CouplingType::PitchToPitch)
            couplingPitchMod += amount;
    }

    float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override {
        return lastOutputSample;
    }

    //--------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer&          midi,
                     int                        numSamples) override
    {
        // 1. Parse MIDI — note-on/off, CC1 (mod wheel), aftertouch
        for (const auto meta : midi) {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn()) {
                currentNote   = msg.getNoteNumber();
                currentVel    = msg.getFloatVelocity();
                noteIsOn      = true;
                ampEnvStage   = EnvStage::Attack;
                // Re-trigger oscillators for mono bass (phase reset on new note)
                oscFund.reset(); oscSub1.reset(); oscSub2.reset();
                wakeSilenceGate();
            } else if (msg.isNoteOff() && msg.getNoteNumber() == currentNote) {
                noteIsOn    = false;
                ampEnvStage = EnvStage::Release;
            } else if (msg.isController() && msg.getControllerNumber() == 1) {
                modWheelVal = msg.getControllerValue() / 127.f; // D006
            } else if (msg.isChannelPressure()) {
                aftertouchVal = msg.getChannelPressureValue() / 127.f; // D006
            } else if (msg.isAftertouch()) {
                aftertouchVal = msg.getAfterTouchValue() / 127.f;      // D006
            }
        }

        // 2. Check SilenceGate bypass
        if (isSilenceGateBypassed()) {
            buffer.clear();
            return;
        }

        // 3. Snapshot parameters once per block (ParamSnapshot pattern)
        if (!p_subLevel) { buffer.clear(); return; } // not yet attached

        const float subLevel      = p_subLevel->load();
        const float subOctMix     = p_subOctMix->load();
        const float baseCutoff    = p_filterCutoff->load();
        const float filterRes     = p_filterRes->load();
        const float velCutoffAmt  = p_velCutoffAmt->load();
        const float bodyFeedback  = p_bodyFeedback->load();
        const float bodyMix       = p_bodyMix->load();
        const int   bodyChar      = (int)(p_bodyChar->load() + 0.5f);
        const float bioRate       = p_bioRate->load();
        const float bioMix        = p_bioMix->load();
        const float bioBrightness = p_bioBrightness->load();
        const float pressureAmt   = p_pressureAmt->load();
        const float ampAtk        = p_ampAtk->load();
        const float ampDec        = p_ampDec->load();
        const float ampSus        = p_ampSus->load();
        const float ampRel        = p_ampRel->load();
        const float lfo1Rate      = p_lfo1Rate->load();
        const float lfo1Depth     = p_lfo1Depth->load();
        const float lfo2Rate      = p_lfo2Rate->load();
        const float lfo2Depth     = p_lfo2Depth->load();
        const float reverbMix     = p_reverbMix->load();

        // 4 macros with their coupling behavior:
        const float macroPressure = p_macroPressure->load();
        const float macroCreature = p_macroCreature->load();
        const float macroWreck    = p_macroWreck->load();
        const float macroAbyss    = p_macroAbyss->load();

        // Macro-derived parameter blends
        // PRESSURE macro: increases pressure amount + pushes sub levels up
        const float effectivePressure = clamp(pressureAmt + macroPressure * 0.4f, 0.f, 1.f);
        const float macroSubBoost     = macroPressure * 0.2f; // boosts sub level

        // CREATURE macro: amplifies bio exciter mix + rate
        const float effectiveBioMix   = clamp(bioMix  + macroCreature * 0.5f, 0.f, 1.f);
        const float effectiveBioRate  = clamp(bioRate + macroCreature * 0.4f, 0.01f, 0.5f);

        // WRECK macro: increases body mix + chooses bodyChar toward wreck
        const float effectiveBodyMix  = clamp(bodyMix + macroWreck * 0.5f, 0.f, 1.f);
        const float effectiveFeedback = clamp(bodyFeedback + macroWreck * 0.2f, 0.f, 0.9f);
        const int   effectiveBodyChar = (macroWreck > 0.7f) ? 2 :
                                        (macroWreck > 0.35f) ? 1 : bodyChar;

        // ABYSS macro: darkens filter (lower cutoff) + increases reverb
        const float abyssFilterClose  = macroAbyss * 350.f; // sweeps cutoff down by up to 350 Hz
        const float effectiveReverb   = clamp(reverbMix + macroAbyss * 0.4f, 0.f, 1.f);

        // Velocity → filter brightness (D001): high velocity = slightly higher cutoff
        const float velCutoffBoost = currentVel * velCutoffAmt * 150.f; // +0..150 Hz

        // Mod wheel → darkness filter (D006)
        const float modWheelCutoffMod = modWheelVal * (-200.f); // lower cutoff

        // Aftertouch → pressure compression (D006)
        const float aftertouchPressureMod = aftertouchVal * 0.3f;
        const float totalPressure = clamp(effectivePressure + aftertouchPressureMod, 0.f, 1.f);

        // Final cutoff (clamped)
        const float finalCutoff = clamp(
            baseCutoff + velCutoffBoost + modWheelCutoffMod
            - abyssFilterClose + couplingFilterMod,
            50.f, 800.f);

        // Resonance → map 0-0.95 to Q factor 0.5 → 12.0
        const float Q = 0.5f + filterRes * 11.5f;

        // Derived frequencies
        const float fundamentalFreq = midiToFreq(currentNote) * fastPow2(couplingPitchMod / 12.f);
        const float sub1Freq        = fundamentalFreq * 0.5f;  // -1 octave
        const float sub2Freq        = fundamentalFreq * 0.25f; // -2 octaves

        // Envelope coefficients (one-pole smoother approach per stage)
        const float atkCoeff  = smoothCoeffFromTime(ampAtk,  sr);
        const float decCoeff  = smoothCoeffFromTime(ampDec,  sr);
        const float relCoeff  = smoothCoeffFromTime(ampRel,  sr);

        // Compressor one-pole coefficients
        const float compAtk  = smoothCoeffFromTime(0.005f, sr); // 5ms attack
        const float compRel  = smoothCoeffFromTime(0.100f, sr); // 100ms release

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;

        // Reset coupling modulation (consumed each block)
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;

        // ----- Per-sample DSP loop -----
        for (int n = 0; n < numSamples; ++n) {
            // --- Amp envelope ---
            float envTarget = 0.f;
            float envCoeff  = 0.f;
            switch (ampEnvStage) {
                case EnvStage::Idle:
                    envTarget = 0.f; envCoeff = relCoeff;
                    break;
                case EnvStage::Attack:
                    envTarget = 1.f; envCoeff = atkCoeff;
                    if (ampEnvLevel >= 0.999f) {
                        ampEnvLevel = 1.f;
                        ampEnvStage = EnvStage::Decay;
                    }
                    break;
                case EnvStage::Decay:
                    envTarget = ampSus; envCoeff = decCoeff;
                    if (std::fabs(ampEnvLevel - ampSus) < 0.001f) {
                        ampEnvLevel = ampSus;
                        ampEnvStage = EnvStage::Sustain;
                    }
                    break;
                case EnvStage::Sustain:
                    envTarget = ampSus; envCoeff = decCoeff;
                    break;
                case EnvStage::Release:
                    envTarget = 0.f; envCoeff = relCoeff;
                    if (ampEnvLevel < 0.0001f) {
                        ampEnvLevel = 0.f;
                        ampEnvStage = EnvStage::Idle;
                    }
                    break;
            }
            ampEnvLevel += envCoeff * (envTarget - ampEnvLevel);
            ampEnvLevel = flushDenormal(ampEnvLevel);

            // --- LFO 1: creature modulation (0.01-2 Hz) ---
            lfo1Phase += lfo1Rate / sr;
            if (lfo1Phase >= 1.f) lfo1Phase -= 1.f;
            float lfo1Val = fastSin(lfo1Phase * 6.2831853f); // sine LFO

            // --- LFO 2: pressure wobble (0.01-0.5 Hz) ---
            lfo2Phase += lfo2Rate / sr;
            if (lfo2Phase >= 1.f) lfo2Phase -= 1.f;
            float lfo2Val = fastSin(lfo2Phase * 6.2831853f); // sine LFO

            // Apply LFO depth
            float lfo1Out = lfo1Val * lfo1Depth;
            float lfo2Out = lfo2Val * lfo2Depth;

            // --- Sub oscillator stack ---
            // LFO2 creates gentle pressure wobble on the fundamental pitch
            float freqMod = fundamentalFreq * (1.f + lfo2Out * 0.005f); // ±0.5% pitch wobble
            float s0 = oscFund.tick(freqMod);
            float s1 = oscSub1.tick(sub1Freq * (1.f + lfo2Out * 0.004f));
            float s2 = oscSub2.tick(sub2Freq * (1.f + lfo2Out * 0.003f));

            // Mix sub stack: fundamental always at full subLevel, subs at scaled levels
            float effectiveSubLevel = clamp(subLevel + macroSubBoost, 0.f, 1.f);
            float subMix  = s0 + s1 * subOctMix + s2 * subOctMix * 0.5f;
            float oscOut  = subMix * effectiveSubLevel;

            // --- Bioluminescent exciter ---
            // LFO1 modulates the bio rate for organic creature-like variation
            float modBioRate = clamp(effectiveBioRate + lfo1Out * 0.1f, 0.01f, 0.5f);
            float bioSample  = bioExciter.tick(modBioRate, bioBrightness, sr);

            // --- Mix osc + bio ---
            float mixedSignal = oscOut * (1.f - effectiveBioMix * 0.5f)
                              + bioSample * effectiveBioMix;

            // --- Waveguide body ---
            float bodySample = body.tick(mixedSignal, freqMod, effectiveFeedback, effectiveBodyChar);
            float withBody   = mixedSignal * (1.f - effectiveBodyMix) + bodySample * effectiveBodyMix;

            // --- Hydrostatic compressor ---
            // LFO2 creates subtle pressure variation (underwater breathing)
            float dynamicPressure = clamp(totalPressure + lfo2Out * lfo2Depth * 0.15f, 0.f, 1.f);
            float compressed      = compressor.process(withBody, dynamicPressure, compAtk, compRel);

            // --- Darkness filter ---
            // LFO1 slightly modulates cutoff for alien life feel
            float dynCutoff = clamp(finalCutoff + lfo1Out * lfo1Depth * 50.f, 50.f, 800.f);
            float filtered  = darknessFilter.process(compressed, dynCutoff, Q, sr);

            // --- Amp envelope ---
            float output = filtered * ampEnvLevel * 0.7f; // 0.7 headroom

            // Soft-clip for subtle saturation under pressure
            output = softClip(output);

            // Write stereo (mono source spread to both channels)
            L[n] = output;
            R[n] = output;

            lastOutputSample = output;
        }

        // --- Block-level reverb processing ---
        // Apply abyssal reverb after the sample loop (block-rate reverb is fine
        // for this type of dense-room tail; no per-sample aliasing concern).
        for (int n = 0; n < numSamples; ++n) {
            float l = L[n], r = R[n];
            // Slight stereo spread: invert a fraction of R for width
            float lIn = l, rIn = r * 0.97f + l * 0.03f;
            reverb.process(lIn, rIn, macroAbyss, effectiveReverb);
            L[n] = lIn;
            R[n] = rIn;
        }

        // 4. Feed SilenceGate analyzer
        analyzeForSilenceGate(buffer, numSamples);
    }

    //--------------------------------------------------------------------------
private:
    // Envelope stages
    enum class EnvStage { Idle, Attack, Decay, Sustain, Release };

    // DSP state
    float sr = 44100.f;
    DeepSineOsc          oscFund, oscSub1, oscSub2;
    DeepHydroCompressor  compressor;
    DeepWaveguideBody    body;
    DeepBioExciter       bioExciter;
    DeepDarknessFilter   darknessFilter;
    DeepAbyssalReverb    reverb;

    // Envelope state
    EnvStage  ampEnvStage = EnvStage::Idle;
    float     ampEnvLevel = 0.f;

    // Voice state
    bool  noteIsOn    = false;
    int   currentNote = 60;
    float currentVel  = 1.f;

    // LFO phases
    float lfo1Phase = 0.f;
    float lfo2Phase = 0.f;

    // Expression
    float modWheelVal   = 0.f;
    float aftertouchVal = 0.f;

    // Coupling modulation (consumed each block)
    float couplingFilterMod = 0.f;
    float couplingPitchMod  = 0.f;

    // Coupling output
    float lastOutputSample = 0.f;

    // Cached parameter pointers (attached once via attachParameters)
    std::atomic<float>* p_subLevel       = nullptr;
    std::atomic<float>* p_subOctMix      = nullptr;
    std::atomic<float>* p_filterCutoff   = nullptr;
    std::atomic<float>* p_filterRes      = nullptr;
    std::atomic<float>* p_velCutoffAmt   = nullptr;
    std::atomic<float>* p_bodyChar       = nullptr;
    std::atomic<float>* p_bodyFeedback   = nullptr;
    std::atomic<float>* p_bodyMix        = nullptr;
    std::atomic<float>* p_bioRate        = nullptr;
    std::atomic<float>* p_bioMix         = nullptr;
    std::atomic<float>* p_bioBrightness  = nullptr;
    std::atomic<float>* p_pressureAmt    = nullptr;
    std::atomic<float>* p_ampAtk         = nullptr;
    std::atomic<float>* p_ampDec         = nullptr;
    std::atomic<float>* p_ampSus         = nullptr;
    std::atomic<float>* p_ampRel         = nullptr;
    std::atomic<float>* p_lfo1Rate       = nullptr;
    std::atomic<float>* p_lfo1Depth      = nullptr;
    std::atomic<float>* p_lfo2Rate       = nullptr;
    std::atomic<float>* p_lfo2Depth      = nullptr;
    std::atomic<float>* p_reverbMix      = nullptr;
    std::atomic<float>* p_macroPressure  = nullptr;
    std::atomic<float>* p_macroCreature  = nullptr;
    std::atomic<float>* p_macroWreck     = nullptr;
    std::atomic<float>* p_macroAbyss     = nullptr;
};

} // namespace xomnibus
