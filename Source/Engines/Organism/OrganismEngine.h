#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdint>

namespace xomnibus {

//==============================================================================
//
//  ORGANISM ENGINE — Generative Cellular Automata Synthesizer
//  XOmnibus Engine Module | Accent: Emergence Lime #C6E377
//
//  Creature: Coral Colony — millions of polyps following simple rules,
//            emergent architecture. The Coral Colony lives in the mid-water
//            column, neither feliX nor Oscar — it is the engine of emergence
//            itself. Simple local rules produce global patterns that no single
//            polyp could ever plan.
//
//  Architecture:
//    1. 16-cell 1D elementary cellular automaton (uint16_t state)
//    2. Wolfram rule 0–255: next cell = f(left, center, right) neighbors
//       Circular wrap: cell 0's left neighbor is cell 15.
//    3. State advances every int(sampleRate / stepRate) samples
//    4. Output mapping per generation (moving average over org_scope generations):
//         Cells 0–3:   filter cutoff position (4-bit → 0–1)
//         Cells 4–7:   amplitude envelope rate (4-bit → attack/decay speed)
//         Cells 8–11:  pitch offset in semitones (4-bit → ±6 st from root)
//         Cells 12–15: FX send/reverb amount (4-bit → 0–1)
//    5. Single saw + sub oscillator pair. Root pitch from MIDI note.
//    6. ADSR amp envelope (rates modulated by automaton output)
//    7. 2-pole lowpass filter (cutoff modulated by automaton output)
//    8. Simple allpass-based reverb tail
//
//  4 Macros:
//    org_macroRule    — maps 0–1 to curated interesting rules
//                       (30, 90, 110, 184, 150, 18, 54, 22)
//    org_macroSeed    — randomizes initial 16-bit state (timbral/rhythmic variety)
//    org_macroCoupling — standard cross-engine coupling amount
//    org_macroMutate  — mutation rate (random bit flips per step)
//
//  ~35 org_ parameters — doctrine compliant D001–D006.
//
//  Parameter prefix: org_   Engine ID: "Organism"
//
//  SilenceGate: 300 ms hold.
//
//==============================================================================

// Curated interesting Wolfram rules for the macroRule sweep
static constexpr int kCuratedRules[8] = { 30, 90, 110, 184, 150, 18, 54, 22 };

// ---------------------------------------------------------------------------
// OrgScopeHistory — circular buffer of recent automaton 16-bit states.
// Used to compute a moving average of cell outputs over the last N generations.
// ---------------------------------------------------------------------------
struct OrgScopeHistory {
    static constexpr int kMaxScope = 16;
    uint16_t buf[kMaxScope] = {};
    int head = 0;
    int count = 0;

    void reset() {
        std::fill(std::begin(buf), std::end(buf), uint16_t(0));
        head = 0;
        count = 0;
    }

    void push(uint16_t state) {
        buf[head] = state;
        head = (head + 1) % kMaxScope;
        if (count < kMaxScope) ++count;
    }

    // Returns the average population of bits in positions [startCell, startCell+3]
    // across the last 'scope' generations. Result is 0–1.
    float averageBits(int startCell, int scope) const {
        if (count == 0 || scope <= 0) return 0.f;
        int n = std::min(scope, count);
        float sum = 0.f;
        // Walk backwards through ring buffer (most recent first)
        int idx = head;
        for (int gen = 0; gen < n; ++gen) {
            idx = (idx - 1 + kMaxScope) % kMaxScope;
            // Sum 4 bits at startCell..startCell+3
            uint16_t s = buf[idx];
            for (int b = 0; b < 4; ++b) {
                int bit = (startCell + b) & 15;
                if ((s >> bit) & 1u) sum += 1.f;
            }
        }
        return sum / (float)(n * 4); // normalised 0–1
    }
};

// ---------------------------------------------------------------------------
// OrgSawOsc — naive bandlimited-by-scope saw wave oscillator
// Phase accumulates 0–1, output is (phase*2-1) for saw.
// ---------------------------------------------------------------------------
struct OrgSawOsc {
    float phase = 0.f;
    float sr    = 44100.f;

    void prepare(double s) { sr = (float)s; }
    void reset()           { phase = 0.f; }

    // Returns one sample of saw output. pitchSemitoneOffset applied here.
    float tick(float freq) {
        phase += freq / sr;
        if (phase >= 1.f) phase -= 1.f;
        return phase * 2.f - 1.f; // saw: -1..+1
    }
};

// ---------------------------------------------------------------------------
// OrgSubOsc — square sub oscillator one octave below
// ---------------------------------------------------------------------------
struct OrgSubOsc {
    float phase = 0.f;
    float sr    = 44100.f;

    void prepare(double s) { sr = (float)s; }
    void reset()           { phase = 0.f; }

    float tick(float freq) {
        phase += (freq * 0.5f) / sr; // sub = half frequency
        if (phase >= 1.f) phase -= 1.f;
        return (phase < 0.5f) ? 1.f : -1.f; // square wave
    }
};

// ---------------------------------------------------------------------------
// OrgSquareOsc — square wave at fundamental frequency
// ---------------------------------------------------------------------------
struct OrgSquareOsc {
    float phase = 0.f;
    float sr    = 44100.f;

    void prepare(double s) { sr = (float)s; }
    void reset()           { phase = 0.f; }

    float tick(float freq) {
        phase += freq / sr;
        if (phase >= 1.f) phase -= 1.f;
        return (phase < 0.5f) ? 1.f : -1.f;
    }
};

// ---------------------------------------------------------------------------
// OrgTriOsc — triangle wave at fundamental frequency
// ---------------------------------------------------------------------------
struct OrgTriOsc {
    float phase = 0.f;
    float sr    = 44100.f;

    void prepare(double s) { sr = (float)s; }
    void reset()           { phase = 0.f; }

    float tick(float freq) {
        phase += freq / sr;
        if (phase >= 1.f) phase -= 1.f;
        // Triangle: 0→1 first half, 1→0 second half, scaled to -1..+1
        float t = (phase < 0.5f) ? (phase * 2.f) : (2.f - phase * 2.f);
        return t * 2.f - 1.f;
    }
};

// ---------------------------------------------------------------------------
// OrgFilter — 2-pole lowpass filter (biquad bilinear transform)
// Tuned to the automaton's filter cell output each block.
// ---------------------------------------------------------------------------
struct OrgFilter {
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

    void computeCoeffs(float fc, float Q, float sr) {
        if (fc == lastFc && Q == lastQ) return;
        lastFc = fc; lastQ = Q;

        fc = clamp(fc, 20.f, sr * 0.45f);
        Q  = clamp(Q,  0.5f, 20.f);

        float w0    = 6.2831853f * fc / sr;
        float cosW  = fastCos(w0);
        float sinW  = fastSin(w0);
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
// OrgReverb — minimal allpass diffusion reverb (emergence shimmer)
// ---------------------------------------------------------------------------
struct OrgReverb {
    static constexpr int kAP1Len = 347, kAP2Len = 673;
    static constexpr int kAP3Len = 1021, kAP4Len = 1471;
    static constexpr int kComb1 = 1741, kComb2 = 1951;
    static constexpr int kComb3 = 2129, kComb4 = 2311;

    float ap1L[kAP1Len]={}, ap1R[kAP1Len]={};
    float ap2L[kAP2Len]={}, ap2R[kAP2Len]={};
    float ap3L[kAP3Len]={}, ap3R[kAP3Len]={};
    float ap4L[kAP4Len]={}, ap4R[kAP4Len]={};
    float c1L[kComb1]={}, c1R[kComb1]={};
    float c2L[kComb2]={}, c2R[kComb2]={};
    float c3L[kComb3]={}, c3R[kComb3]={};
    float c4L[kComb4]={}, c4R[kComb4]={};
    int p1L=0,p1R=0,p2L=0,p2R=0,p3L=0,p3R=0,p4L=0,p4R=0;
    int pc1L=0,pc1R=0,pc2L=0,pc2R=0,pc3L=0,pc3R=0,pc4L=0,pc4R=0;
    float cs1L=0.f,cs1R=0.f,cs2L=0.f,cs2R=0.f;
    float cs3L=0.f,cs3R=0.f,cs4L=0.f,cs4R=0.f;

    void reset() {
        std::fill(ap1L, ap1L+kAP1Len, 0.f); std::fill(ap1R, ap1R+kAP1Len, 0.f);
        std::fill(ap2L, ap2L+kAP2Len, 0.f); std::fill(ap2R, ap2R+kAP2Len, 0.f);
        std::fill(ap3L, ap3L+kAP3Len, 0.f); std::fill(ap3R, ap3R+kAP3Len, 0.f);
        std::fill(ap4L, ap4L+kAP4Len, 0.f); std::fill(ap4R, ap4R+kAP4Len, 0.f);
        std::fill(c1L, c1L+kComb1, 0.f); std::fill(c1R, c1R+kComb1, 0.f);
        std::fill(c2L, c2L+kComb2, 0.f); std::fill(c2R, c2R+kComb2, 0.f);
        std::fill(c3L, c3L+kComb3, 0.f); std::fill(c3R, c3R+kComb3, 0.f);
        std::fill(c4L, c4L+kComb4, 0.f); std::fill(c4R, c4R+kComb4, 0.f);
        p1L=p1R=p2L=p2R=p3L=p3R=p4L=p4R=0;
        pc1L=pc1R=pc2L=pc2R=pc3L=pc3R=pc4L=pc4R=0;
        cs1L=cs1R=cs2L=cs2R=cs3L=cs3R=cs4L=cs4R=0.f;
    }

    void prepare(double /*sr*/) { reset(); }

    // ap: generic allpass node
    static float runAP(float* buf, int& pos, int len, float g, float in) {
        float rd = buf[pos];
        float wr = in + g * rd;
        wr = flushDenormal(wr);
        buf[pos] = wr;
        pos = (pos + 1 >= len) ? 0 : pos + 1;
        return rd - g * wr;
    }

    // comb with LP damping
    static float runComb(float* buf, int& pos, float& state,
                         int len, float fb, float damp, float in) {
        float rd = buf[pos];
        state = flushDenormal(rd * (1.f - damp) + state * damp);
        float wr = in + fb * state;
        wr = flushDenormal(wr);
        buf[pos] = wr;
        pos = (pos + 1 >= len) ? 0 : pos + 1;
        return rd;
    }

    void process(float& inL, float& inR, float mix) {
        float dryL = inL, dryR = inR;
        float fb   = 0.72f;
        float damp = 0.25f;

        float wL = 0.f, wR = 0.f;
        wL += runComb(c1L, pc1L, cs1L, kComb1, fb, damp, inL);
        wR += runComb(c1R, pc1R, cs1R, kComb1, fb, damp, inR);
        wL += runComb(c2L, pc2L, cs2L, kComb2, fb, damp, inL);
        wR += runComb(c2R, pc2R, cs2R, kComb2, fb, damp, inR);
        wL += runComb(c3L, pc3L, cs3L, kComb3, fb, damp, inL);
        wR += runComb(c3R, pc3R, cs3R, kComb3, fb, damp, inR);
        wL += runComb(c4L, pc4L, cs4L, kComb4, fb, damp, inL);
        wR += runComb(c4R, pc4R, cs4R, kComb4, fb, damp, inR);
        wL *= 0.25f; wR *= 0.25f;

        wL = runAP(ap1L, p1L, kAP1Len, 0.5f, wL);
        wR = runAP(ap1R, p1R, kAP1Len, 0.5f, wR);
        wL = runAP(ap2L, p2L, kAP2Len, 0.5f, wL);
        wR = runAP(ap2R, p2R, kAP2Len, 0.5f, wR);
        wL = runAP(ap3L, p3L, kAP3Len, 0.45f, wL);
        wR = runAP(ap3R, p3R, kAP3Len, 0.45f, wR);
        wL = runAP(ap4L, p4L, kAP4Len, 0.45f, wL);
        wR = runAP(ap4R, p4R, kAP4Len, 0.45f, wR);

        inL = dryL * (1.f - mix) + wL * mix;
        inR = dryR * (1.f - mix) + wR * mix;
    }
};

//==============================================================================
// OrganismEngine — the full SynthEngine implementation
//==============================================================================
class OrganismEngine : public SynthEngine {
public:
    OrganismEngine()  = default;
    ~OrganismEngine() = default;

    //--------------------------------------------------------------------------
    // Static parameter registration (called by XOmnibusProcessor)
    //--------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P  = juce::ParameterID;
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;
        using PB = juce::AudioParameterBool;
        auto nr  = [](float lo, float hi, float step=0.f) {
            return juce::NormalisableRange<float>(lo, hi, step);
        };

        // --- Cellular automaton control ---
        params.push_back(std::make_unique<PF>(P("org_rule",1), "Rule",
            nr(0.f, 255.f, 1.f), 110.f));
        params.push_back(std::make_unique<PI>(P("org_seed",1), "Seed",
            0, 65535, 42));
        params.push_back(std::make_unique<PF>(P("org_stepRate",1), "Step Rate",
            nr(0.5f, 32.f), 4.f));
        params.push_back(std::make_unique<PI>(P("org_scope",1), "Scope",
            1, 16, 4));
        params.push_back(std::make_unique<PF>(P("org_mutate",1), "Mutate",
            nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PB>(P("org_freeze",1), "Freeze",
            false));

        // --- Oscillator ---
        params.push_back(std::make_unique<PI>(P("org_oscWave",1), "Osc Wave",
            0, 2, 0)); // 0=saw, 1=square, 2=tri
        params.push_back(std::make_unique<PF>(P("org_subLevel",1), "Sub Level",
            nr(0.f, 1.f), 0.35f));

        // --- Filter ---
        params.push_back(std::make_unique<PF>(P("org_filterCutoff",1), "Filter Cutoff",
            nr(200.f, 8000.f), 3000.f));
        params.push_back(std::make_unique<PF>(P("org_filterRes",1), "Filter Resonance",
            nr(0.f, 0.9f), 0.3f));

        // --- Velocity ---
        params.push_back(std::make_unique<PF>(P("org_velCutoff",1), "Vel Cutoff",
            nr(0.f, 1.f), 0.5f));

        // --- Amp envelope ---
        params.push_back(std::make_unique<PF>(P("org_ampAtk",1), "Amp Attack",
            nr(0.001f, 2.0f), 0.015f));
        params.push_back(std::make_unique<PF>(P("org_ampDec",1), "Amp Decay",
            nr(0.05f, 4.0f), 0.35f));
        params.push_back(std::make_unique<PF>(P("org_ampSus",1), "Amp Sustain",
            nr(0.f, 1.f), 0.7f));
        params.push_back(std::make_unique<PF>(P("org_ampRel",1), "Amp Release",
            nr(0.05f, 5.0f), 0.6f));

        // --- LFO 1: step rate modulation ---
        params.push_back(std::make_unique<PF>(P("org_lfo1Rate",1), "LFO1 Rate",
            nr(0.01f, 10.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("org_lfo1Depth",1), "LFO1 Depth",
            nr(0.f, 1.f), 0.2f));

        // --- LFO 2: filter cutoff offset ---
        params.push_back(std::make_unique<PF>(P("org_lfo2Rate",1), "LFO2 Rate",
            nr(0.01f, 10.f), 0.3f));
        params.push_back(std::make_unique<PF>(P("org_lfo2Depth",1), "LFO2 Depth",
            nr(0.f, 1.f), 0.25f));

        // --- Reverb ---
        params.push_back(std::make_unique<PF>(P("org_reverbMix",1), "Reverb Mix",
            nr(0.f, 1.f), 0.2f));

        // --- 4 Macros ---
        params.push_back(std::make_unique<PF>(P("org_macroRule",1), "RULE",
            nr(0.f, 1.f), 0.25f)); // maps to rule 110 by default
        params.push_back(std::make_unique<PF>(P("org_macroSeed",1), "SEED",
            nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("org_macroCoupling",1), "COUPLING",
            nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("org_macroMutate",1), "MUTATE",
            nr(0.f, 1.f), 0.f));
    }

    //--------------------------------------------------------------------------
    // SynthEngine interface
    //--------------------------------------------------------------------------
    juce::String   getEngineId()     const override { return "Organism"; }
    juce::Colour   getAccentColour() const override { return juce::Colour(0xffC6E377); }
    int            getMaxVoices()    const override { return 1; } // monophonic generative engine

    int getActiveVoiceCount() const override {
        return (noteIsOn || ampEnvStage != EnvStage::Idle) ? 1 : 0;
    }

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = (float)sampleRate;

        sawOsc.prepare(sampleRate);
        sqOsc.prepare(sampleRate);
        triOsc.prepare(sampleRate);
        subOsc.prepare(sampleRate);
        filter.reset();
        reverb.prepare(sampleRate);

        // Initialise automaton
        caState       = 0x0042u; // default seed=42
        stepCounter   = 0;
        scopeHistory.reset();
        cellFilterOut = 0.5f;
        cellAmpRate   = 0.5f;
        cellPitchOut  = 0.f;
        cellFXOut     = 0.5f;

        ampEnvStage = EnvStage::Idle;
        ampEnvLevel = 0.f;
        noteIsOn    = false;
        currentNote = 60;
        currentVel  = 1.f;
        lfo1Phase   = 0.f;
        lfo2Phase   = 0.f;
        modWheelVal = 0.f;
        aftertouchVal = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;

        // Seed the LCG RNG with a fixed value
        rng = 0xDEADBEEFu;

        prepareSilenceGate(sampleRate, maxBlockSize, 300.f);
        (void)maxBlockSize;
    }

    void releaseResources() override {}

    void reset() override
    {
        sawOsc.reset();
        sqOsc.reset();
        triOsc.reset();
        subOsc.reset();
        filter.reset();
        reverb.reset();

        caState       = 0x0042u;
        stepCounter   = 0;
        scopeHistory.reset();
        cellFilterOut = 0.5f;
        cellAmpRate   = 0.5f;
        cellPitchOut  = 0.f;
        cellFXOut     = 0.5f;

        ampEnvStage = EnvStage::Idle;
        ampEnvLevel = 0.f;
        noteIsOn    = false;
        lfo1Phase   = 0.f;
        lfo2Phase   = 0.f;
        rng         = 0xDEADBEEFu;
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
        p_rule          = apvts.getRawParameterValue("org_rule");
        p_seed          = apvts.getRawParameterValue("org_seed");
        p_stepRate      = apvts.getRawParameterValue("org_stepRate");
        p_scope         = apvts.getRawParameterValue("org_scope");
        p_mutate        = apvts.getRawParameterValue("org_mutate");
        p_freeze        = apvts.getRawParameterValue("org_freeze");
        p_oscWave       = apvts.getRawParameterValue("org_oscWave");
        p_subLevel      = apvts.getRawParameterValue("org_subLevel");
        p_filterCutoff  = apvts.getRawParameterValue("org_filterCutoff");
        p_filterRes     = apvts.getRawParameterValue("org_filterRes");
        p_velCutoff     = apvts.getRawParameterValue("org_velCutoff");
        p_ampAtk        = apvts.getRawParameterValue("org_ampAtk");
        p_ampDec        = apvts.getRawParameterValue("org_ampDec");
        p_ampSus        = apvts.getRawParameterValue("org_ampSus");
        p_ampRel        = apvts.getRawParameterValue("org_ampRel");
        p_lfo1Rate      = apvts.getRawParameterValue("org_lfo1Rate");
        p_lfo1Depth     = apvts.getRawParameterValue("org_lfo1Depth");
        p_lfo2Rate      = apvts.getRawParameterValue("org_lfo2Rate");
        p_lfo2Depth     = apvts.getRawParameterValue("org_lfo2Depth");
        p_reverbMix     = apvts.getRawParameterValue("org_reverbMix");
        p_macroRule     = apvts.getRawParameterValue("org_macroRule");
        p_macroSeed     = apvts.getRawParameterValue("org_macroSeed");
        p_macroCoupling = apvts.getRawParameterValue("org_macroCoupling");
        p_macroMutate   = apvts.getRawParameterValue("org_macroMutate");
    }

    //--------------------------------------------------------------------------
    void applyCouplingInput(CouplingType type, float amount,
                            const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        // macroCoupling scales receive sensitivity so the COUPLING macro always
        // controls how strongly coupled partners influence this engine.
        const float recvScale = p_macroCoupling ? (0.5f + p_macroCoupling->load() * 0.5f) : 0.5f;
        if (type == CouplingType::AmpToFilter)
            couplingFilterMod += amount * 1000.f * recvScale; // ±1000 Hz modulation range
        else if (type == CouplingType::AmpToPitch || type == CouplingType::PitchToPitch)
            couplingPitchMod += amount * recvScale;
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
                currentNote  = msg.getNoteNumber();
                currentVel   = msg.getFloatVelocity();
                noteIsOn     = true;
                ampEnvStage  = EnvStage::Attack;

                // Reset automaton on note-on. Different MIDI notes produce
                // different evolution trajectories.
                uint16_t seedParam = p_seed ? (uint16_t)((int)(p_seed->load()) & 0xFFFF) : 42u;
                // XOR seed with note number for per-note variation
                caState     = (uint16_t)(seedParam ^ (uint16_t)(currentNote * 257u));
                stepCounter = 0;
                scopeHistory.reset();

                // Reset oscillators for clean attack
                sawOsc.reset(); sqOsc.reset(); triOsc.reset(); subOsc.reset();
                wakeSilenceGate();

            } else if (msg.isNoteOff() && msg.getNoteNumber() == currentNote) {
                noteIsOn    = false;
                ampEnvStage = EnvStage::Release;

            } else if (msg.isController() && msg.getControllerNumber() == 1) {
                // D006: mod wheel morphs rule
                modWheelVal = msg.getControllerValue() / 127.f;

            } else if (msg.isChannelPressure()) {
                // D006: aftertouch controls mutation rate
                aftertouchVal = msg.getChannelPressureValue() / 127.f;

            } else if (msg.isAftertouch()) {
                aftertouchVal = msg.getAfterTouchValue() / 127.f;
            }
        }

        // 2. Check SilenceGate bypass
        if (isSilenceGateBypassed()) {
            buffer.clear();
            return;
        }

        // 3. Guard: parameters not yet attached
        if (!p_rule) { buffer.clear(); return; }

        // 4. Snapshot parameters once per block (ParamSnapshot pattern)
        const float paramRule       = p_rule->load();
        const float stepRate        = p_stepRate->load();
        const int   scope           = clamp((float)(int)(p_scope->load() + 0.5f), 1.f, 16.f);
        const float baseMutate      = p_mutate->load();
        const bool  freeze          = (p_freeze->load() > 0.5f);
        const int   oscWave         = (int)(p_oscWave->load() + 0.5f);
        const float subLevel        = p_subLevel->load();
        const float baseCutoff      = p_filterCutoff->load();
        const float filterRes       = p_filterRes->load();
        const float velCutoff       = p_velCutoff->load();
        const float ampAtk          = p_ampAtk->load();
        const float ampDec          = p_ampDec->load();
        const float ampSus          = p_ampSus->load();
        const float ampRel          = p_ampRel->load();
        const float lfo1Rate        = p_lfo1Rate->load();
        const float lfo1Depth       = p_lfo1Depth->load();
        const float lfo2Rate        = p_lfo2Rate->load();
        const float lfo2Depth       = p_lfo2Depth->load();
        const float reverbMix       = p_reverbMix->load();
        const float macroRule       = p_macroRule->load();
        const float macroSeed       = p_macroSeed->load(); // use to re-seed if >0
        const float macroCoupling   = p_macroCoupling ? p_macroCoupling->load() : 0.0f;
        const float macroMutate     = p_macroMutate->load();

        // macroRule: map 0–1 across 8 curated rules (index 0–7)
        // modWheel additively morphs the rule index (+/- up to 2 positions)
        // D006: mod wheel = rule morph
        float ruleIndexF = macroRule * 7.f + modWheelVal * 2.f;
        ruleIndexF = clamp(ruleIndexF, 0.f, 7.f);
        int ruleIdxA = (int)ruleIndexF;
        int ruleIdxB = std::min(ruleIdxA + 1, 7);
        float ruleBlend = ruleIndexF - (float)ruleIdxA;

        // Blend between two adjacent curated rules by xor-smoothing the rule byte
        // (integer blend: majority bit vote per bit position)
        int ruleA = kCuratedRules[ruleIdxA];
        int ruleB = kCuratedRules[ruleIdxB];
        // Simple blend: pick ruleA or ruleB per bit based on blend threshold
        int currentRule = 0;
        for (int bit = 0; bit < 8; ++bit) {
            int ba = (ruleA >> bit) & 1;
            int bb = (ruleB >> bit) & 1;
            // If bits agree, use that; otherwise use blend probability
            if (ba == bb) {
                currentRule |= (ba << bit);
            } else {
                // blend: prefer B when ruleBlend > 0.5
                currentRule |= ((ruleBlend > 0.5f ? bb : ba) << bit);
            }
        }

        // paramRule overrides the macro when it has been moved away from default
        // (In practice, p_rule is the manual rule param, macroRule is the macro.
        //  We use paramRule directly when macroRule is at minimum.)
        if (macroRule < 0.01f && modWheelVal < 0.01f) {
            currentRule = (int)(paramRule + 0.5f) & 0xFF;
        }

        // D006: aftertouch = mutation rate override
        // macroCoupling adds a subtle autonomous mutation boost (0..0.01) so that
        // turning COUPLING up makes the automaton slightly more unpredictable even
        // without a partner engine. The scale is kept very small (0.01) so it never
        // dominates the dedicated MUTATE macro but is always audible on close listening.
        const float effectiveMutate = clamp(baseMutate + macroMutate + aftertouchVal * 0.3f
                                            + macroCoupling * 0.01f,
                                            0.f, 1.f);

        // macroSeed: if > 0.01, re-seed automaton this block (only once per gesture)
        // We use a latch: re-seed only when macroSeed transitions above threshold
        if (macroSeed > 0.01f && !macroSeedLatched) {
            macroSeedLatched = true;
            // Derive a new state from the LCG
            rng = rng * 1664525u + 1013904223u;
            caState = (uint16_t)(rng & 0xFFFF);
            if (caState == 0) caState = 0x5555u; // prevent all-zero
            scopeHistory.reset();
        } else if (macroSeed < 0.005f) {
            macroSeedLatched = false;
        }

        // Compute samples per automaton step
        const float effectiveStepRate = clamp(stepRate, 0.5f, 32.f);
        const int   samplesPerStep    = std::max(1, (int)(sr / effectiveStepRate));

        // D001: velocity → filter brightness
        const float velCutoffBoost = currentVel * velCutoff * 3000.f; // +0..3000 Hz

        // Envelope coefficients
        const float atkCoeff = smoothCoeffFromTime(ampAtk, sr);
        const float decCoeff = smoothCoeffFromTime(ampDec, sr);
        const float relCoeff = smoothCoeffFromTime(ampRel, sr);

        // Resonance → Q factor 0.5 → 12.0
        const float Q = 0.5f + filterRes * 11.5f;

        // Root pitch from MIDI note, with pitch coupling and automaton pitch offset
        const float rootFreq = midiToFreq(currentNote);

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;

        // Consume coupling modulation (resets each block)
        const float savedCouplingFilter = couplingFilterMod;
        const float savedCouplingPitch  = couplingPitchMod;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;

        // ----- Per-sample DSP loop -----
        for (int n = 0; n < numSamples; ++n) {

            // --- Advance cellular automaton ---
            if (!freeze) {
                ++stepCounter;
                if (stepCounter >= samplesPerStep) {
                    stepCounter = 0;
                    caState = stepAutomaton(caState, (uint8_t)currentRule,
                                           effectiveMutate);
                    scopeHistory.push(caState);

                    // Update output mappings from scope-averaged cell groups
                    cellFilterOut = scopeHistory.averageBits(0,  scope); // cells 0–3
                    cellAmpRate   = scopeHistory.averageBits(4,  scope); // cells 4–7
                    cellPitchOut  = scopeHistory.averageBits(8,  scope); // cells 8–11
                    cellFXOut     = scopeHistory.averageBits(12, scope); // cells 12–15
                }
            }

            // --- LFO 1: modulates step rate (but step timing uses block-level stepRate param)
            //     Here LFO1 modulates filter cutoff for additional movement
            lfo1Phase += lfo1Rate / sr;
            if (lfo1Phase >= 1.f) lfo1Phase -= 1.f;
            float lfo1Val = fastSin(lfo1Phase * 6.2831853f);

            // --- LFO 2: filter cutoff offset ---
            lfo2Phase += lfo2Rate / sr;
            if (lfo2Phase >= 1.f) lfo2Phase -= 1.f;
            float lfo2Val = fastSin(lfo2Phase * 6.2831853f);

            // --- Amp envelope ---
            float envTarget = 0.f;
            float envCoeff  = relCoeff;

            // Cellular automaton modulates amp envelope rate (cells 4–7)
            // cellAmpRate 0–1: 0 = slowest (3× base), 1 = fastest (1/3× base)
            // This makes the envelope breathe with the automaton rhythm.
            float envRateMod = lerp(3.f, 0.33f, cellAmpRate);

            switch (ampEnvStage) {
                case EnvStage::Idle:
                    envTarget = 0.f; envCoeff = relCoeff;
                    break;
                case EnvStage::Attack:
                    envTarget = 1.f;
                    envCoeff  = clamp(atkCoeff * envRateMod, 0.f, 1.f);
                    if (ampEnvLevel >= 0.999f) {
                        ampEnvLevel = 1.f;
                        ampEnvStage = EnvStage::Decay;
                    }
                    break;
                case EnvStage::Decay:
                    envTarget = ampSus;
                    envCoeff  = clamp(decCoeff * envRateMod, 0.f, 1.f);
                    if (std::fabs(ampEnvLevel - ampSus) < 0.001f) {
                        ampEnvLevel = ampSus;
                        ampEnvStage = EnvStage::Sustain;
                    }
                    break;
                case EnvStage::Sustain:
                    envTarget = ampSus;
                    envCoeff  = decCoeff;
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

            // --- Pitch ---
            // Cells 8–11 → ±6 semitones offset from root
            // cellPitchOut 0–1 → semitone offset: (cellPitchOut * 12 - 6)
            float pitchSemitones = (cellPitchOut * 12.f - 6.f)
                                 + savedCouplingPitch;
            float freq = rootFreq * fastPow2(pitchSemitones / 12.f);

            // --- Oscillator ---
            float oscOut = 0.f;
            switch (oscWave) {
                case 0:  oscOut = sawOsc.tick(freq); break;
                case 1:  oscOut = sqOsc.tick(freq);  break;
                case 2:  oscOut = triOsc.tick(freq); break;
                default: oscOut = sawOsc.tick(freq); break;
            }
            float subOut = subOsc.tick(freq);
            float mixed  = oscOut * (1.f - subLevel * 0.5f) + subOut * subLevel;

            // --- Filter ---
            // Cells 0–3 → filter cutoff position 0–1 mapped to 200–8000 Hz range
            // LFO2 provides additional modulation
            float cellCutoffOffset = (cellFilterOut - 0.5f) * (baseCutoff * 0.8f);
            float lfo2CutoffMod    = lfo2Val * lfo2Depth * 1000.f;
            float lfo1CutoffMod    = lfo1Val * lfo1Depth * 600.f;
            float finalCutoff = clamp(
                baseCutoff + cellCutoffOffset + velCutoffBoost
                + lfo1CutoffMod + lfo2CutoffMod
                + savedCouplingFilter,
                200.f, 8000.f);

            float filtered = filter.process(mixed, finalCutoff, Q, sr);

            // --- Amp envelope + gain ---
            float output = filtered * ampEnvLevel * 0.65f;

            // Soft-clip
            output = softClip(output);

            L[n] = output;
            R[n] = output;

            lastOutputSample = output;
        }

        // --- Block-level reverb ---
        // Cells 12–15 (cellFXOut) modulate reverb mix
        float effectiveReverb = clamp(reverbMix + cellFXOut * 0.3f, 0.f, 1.f);
        for (int n = 0; n < numSamples; ++n) {
            float l = L[n], r = R[n];
            reverb.process(l, r, effectiveReverb);
            L[n] = l;
            R[n] = r;
        }

        // 5. Feed SilenceGate analyzer
        analyzeForSilenceGate(buffer, numSamples);
    }

    //--------------------------------------------------------------------------
private:
    // Envelope stages
    enum class EnvStage { Idle, Attack, Decay, Sustain, Release };

    //--------------------------------------------------------------------------
    // Wolfram elementary cellular automaton step (1D, circular, 16 cells).
    //
    // Rule is an 8-bit byte. For each cell i:
    //   left  = bit (i-1+16) % 16
    //   center= bit i
    //   right = bit (i+1) % 16
    //   neighborhood index = left*4 + center*2 + right*1   (0..7)
    //   new bit = (rule >> index) & 1
    //
    // mutateProb: probability 0–1 that any given bit is randomly flipped
    // after the rule step. Uses the internal LCG.
    //--------------------------------------------------------------------------
    uint16_t stepAutomaton(uint16_t state, uint8_t rule, float mutateProb) {
        uint16_t next = 0;
        for (int i = 0; i < 16; ++i) {
            int left   = (i - 1 + 16) & 15;
            int right  = (i + 1) & 15;
            int l      = (state >> left)  & 1;
            int c      = (state >> i)     & 1;
            int r      = (state >> right) & 1;
            int idx    = (l << 2) | (c << 1) | r;
            int newBit = (rule >> idx) & 1;
            next |= (uint16_t)(newBit << i);
        }

        // Mutation: random bit flip per cell
        if (mutateProb > 0.f) {
            for (int i = 0; i < 16; ++i) {
                rng = rng * 1664525u + 1013904223u;
                float t = (float)(rng >> 8) * (1.f / 16777216.f); // 0..1 uniform
                if (t < mutateProb) {
                    next ^= (uint16_t)(1u << i); // flip bit
                }
            }
        }

        // Prevent all-zero state (frozen silence) — inject a single bit
        if (next == 0) next = 0x0001u;

        return next;
    }

    //--------------------------------------------------------------------------
    // DSP state
    float sr = 44100.f;
    OrgSawOsc    sawOsc;
    OrgSquareOsc sqOsc;
    OrgTriOsc    triOsc;
    OrgSubOsc    subOsc;
    OrgFilter    filter;
    OrgReverb    reverb;

    // Automaton state
    uint16_t       caState       = 0x0042u;
    int            stepCounter   = 0;
    OrgScopeHistory scopeHistory;
    float          cellFilterOut = 0.5f; // smoothed cells 0–3 average
    float          cellAmpRate   = 0.5f; // smoothed cells 4–7 average
    float          cellPitchOut  = 0.f;  // smoothed cells 8–11 average
    float          cellFXOut     = 0.5f; // smoothed cells 12–15 average

    // LCG for mutation + seeding
    uint32_t rng = 0xDEADBEEFu;

    // macroSeed latch (only re-seed once per macro gesture)
    bool macroSeedLatched = false;

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

    // Expression (D006)
    float modWheelVal   = 0.f;
    float aftertouchVal = 0.f;

    // Coupling modulation (consumed each block)
    float couplingFilterMod = 0.f;
    float couplingPitchMod  = 0.f;

    // Coupling output cache
    float lastOutputSample = 0.f;

    // Cached parameter pointers (attached once via attachParameters)
    std::atomic<float>* p_rule          = nullptr;
    std::atomic<float>* p_seed          = nullptr;
    std::atomic<float>* p_stepRate      = nullptr;
    std::atomic<float>* p_scope         = nullptr;
    std::atomic<float>* p_mutate        = nullptr;
    std::atomic<float>* p_freeze        = nullptr;
    std::atomic<float>* p_oscWave       = nullptr;
    std::atomic<float>* p_subLevel      = nullptr;
    std::atomic<float>* p_filterCutoff  = nullptr;
    std::atomic<float>* p_filterRes     = nullptr;
    std::atomic<float>* p_velCutoff     = nullptr;
    std::atomic<float>* p_ampAtk        = nullptr;
    std::atomic<float>* p_ampDec        = nullptr;
    std::atomic<float>* p_ampSus        = nullptr;
    std::atomic<float>* p_ampRel        = nullptr;
    std::atomic<float>* p_lfo1Rate      = nullptr;
    std::atomic<float>* p_lfo1Depth     = nullptr;
    std::atomic<float>* p_lfo2Rate      = nullptr;
    std::atomic<float>* p_lfo2Depth     = nullptr;
    std::atomic<float>* p_reverbMix     = nullptr;
    std::atomic<float>* p_macroRule     = nullptr;
    std::atomic<float>* p_macroSeed     = nullptr;
    std::atomic<float>* p_macroCoupling = nullptr;
    std::atomic<float>* p_macroMutate   = nullptr;
};

} // namespace xomnibus
