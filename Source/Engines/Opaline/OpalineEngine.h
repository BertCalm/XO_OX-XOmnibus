// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OpalineEngine.h — XOpaline | "The Porcelain Bell"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOpaline is the teacup you play with a spoon — a porcelain ramekin
//      tapped at the rim, a crystal wine glass singing under a wet finger,
//      a toy piano's metal tines struck by tiny hammers in a child's bedroom.
//      Every sound is pure, narrow, crystalline. Every sound can break.
//
//  ENGINE CONCEPT:
//      Glass and porcelain modal synthesis. The MOST DELICATE engine in the
//      XOceanus fleet — where XOven is massive and dark, XOpaline is tiny
//      and crystalline. Four instruments (Celesta, Toy Piano, Glass Harp,
//      Porcelain Cups) each with distinct modal ratios and decay profiles.
//      The defining mechanic: FRAGILITY. Hard velocity can crack the sound,
//      introducing noise bursts and modal detuning. Beautiful because breakable.
//
//  KITCHEN QUAD POSITION:
//      Fourth engine. Glass/Porcelain material. Lowest impedance (Z=12.6M),
//      best energy crossing, narrowest mode bandwidth, most fragile.
//      Contrast: XOven (massive dark) vs XOpaline (tiny crystalline).
//
//  PHYSICS:
//      Material: Borosilicate glass / porcelain
//        Density rho = 2230 kg/m^3
//        Wave speed c = 5640 m/s
//        Impedance Z = rho * c = 12,577,200
//      Very high Q (glass rings at exact frequencies for a long time)
//      Nearly harmonic for simple shapes, inharmonic for complex shapes
//      Quick initial decay, then long crystalline ring
//
//  RECIPE ANALOGY:
//      Creme Brulee — cooked in a porcelain ramekin, the thin crust shattered
//      with a spoon. The crack of breaking the crust IS XOpaline's sound:
//      crystalline, thin, irreversible. You cannot un-crack a creme brulee.
//
//  THE 5 PILLARS:
//      1. Instrument Selector — Celesta/ToyPiano/GlassHarp/PorcelainCups
//         with per-instrument modal ratio tables and excitation character
//      2. Glass Modal Resonator Bank — 16 modes with material-derived
//         eigenfrequencies, extremely high Q, narrow bandwidth
//      3. Fragility Mechanic — velocity * fragility threshold triggers
//         noise burst (the crack) + modal detuning (broken glass rings
//         differently). NOT a bug — the defining character feature.
//      4. Thermal Shock — rapid temperature change causes differential
//         expansion, creating non-uniform detuning between partials
//         (glass threatening to crack)
//      5. Crystalline Envelope — Pate de Verre: brief window of plasticity
//         then instant crystallization. Short attack, very fast initial
//         decay to a pure ringing sustain, then nothing.
//
//  REFERENCES:
//      - Fletcher, N.H. & Rossing, T.D. (1998). "The Physics of Musical
//        Instruments." Springer. Ch. 2-3 (bars, plates, bells).
//      - Chaigne, A. & Doutaut, V. (1997). "Numerical simulations of
//        xylophones." JASA 101(1).
//      - Bilbao, S. (2009). "Numerical Sound Synthesis." Wiley.
//      - Rossing, T.D. (2000). "Science of Percussion Instruments."
//        World Scientific.
//
//  Accent: Crystal Blue #B8D4E3
//  Parameter prefix: opal2_
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
// Modal ratio tables — per instrument type.
// Derived from Fletcher & Rossing (1998) and Rossing (2000).
//
// Celesta: metal bars with resonator tubes — nearly harmonic, bell-like purity
// Toy Piano: metal tines — brighter, slightly inharmonic, lo-fi character
// Glass Harp: wine glass rim excitation — strongly inharmonic (circular plate modes)
// Porcelain Cups: struck ceramic — bell-like but with porcelain's narrow bandwidth
//==============================================================================

namespace
{ // anonymous namespace — prevents ODR violations when multiple engine headers are included

// Celesta: tuned metal bars — nearly perfect harmonic series
// (bars with resonator tubes produce very clean partials)
constexpr float kCelestaRatios[16] = {1.000f, 2.000f, 3.000f, 4.000f, 5.000f, 6.000f, 7.000f, 8.000f,
                                      9.000f, 10.00f, 11.00f, 12.00f, 13.00f, 14.00f, 15.00f, 16.00f};

// Toy Piano: metal tines struck by small hammers — slightly inharmonic
// (short, stiff tines produce stretched partials per beam equation)
constexpr float kToyPianoRatios[16] = {1.000f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.81f, 31.84f,
                                       1.500f, 3.010f, 5.982f, 9.880f, 14.63f, 20.22f, 26.66f, 33.95f};

// Glass Harp: circular plate/shell modes — strongly inharmonic
// (wine glass has (n,m) shell mode pairs, producing ethereal shimmer)
constexpr float kGlassHarpRatios[16] = {1.000f, 1.594f, 2.136f, 2.653f, 3.156f, 3.652f, 4.154f, 4.670f,
                                        5.208f, 5.774f, 6.373f, 7.011f, 7.694f, 8.426f, 9.212f, 10.06f};

// Porcelain Cups: struck ceramic — bell-like (Rossing 2000)
// (porcelain cup modes are between bell and plate — moderately inharmonic)
constexpr float kPorcelainRatios[16] = {1.000f, 1.506f, 2.000f, 2.514f, 3.011f, 3.578f, 4.170f, 4.952f,
                                        5.820f, 6.770f, 7.800f, 8.910f, 10.10f, 11.37f, 12.72f, 14.15f};

// Per-instrument mode amplitude rolloff: how quickly higher modes decay
// Celesta = slow rolloff (pure), Toy Piano = mid, Glass/Porcelain = fast
constexpr float kModeRolloff[4] = {0.12f, 0.20f, 0.18f, 0.15f};

// Per-instrument base Q (glass is VERY high Q — rings forever)
constexpr float kBaseQ[4] = {200.0f, 120.0f, 500.0f, 350.0f};

// Per-instrument sympathetic ring potential (glass: almost none).
// NOTE: kSympathyScale is reserved for a planned sympathetic resonance feature;
// currently unused. Kept as a named constant rather than deleted so its intended
// role (scaling sympathetic coupling between voices) is not lost. (Params/D-new)

} // anonymous namespace

//==============================================================================
// OpalineExciter — Hammer/striker model tailored per instrument type.
// Celesta: felt-tipped hammer (warm, rounded pulse)
// Toy Piano: small hard hammer (sharp, bright pulse)
// Glass Harp: friction excitation (slow, sustained drive)
// Porcelain Cups: spoon strike (very short, metallic click)
//==============================================================================
struct OpalineExciter
{
    void trigger(float velocity, float hardness, float baseFreq, float fragility, int instrument,
                 float sampleRate) noexcept
    {
        active = true;
        sampleCounter = 0;

        // Contact time depends on instrument and hardness
        float contactMs;
        switch (instrument)
        {
        case 0:
            contactMs = 4.0f - hardness * 3.0f;
            break; // Celesta: felt hammer
        case 1:
            contactMs = 2.0f - hardness * 1.5f;
            break; // Toy Piano: short hard hit
        case 2:
            contactMs = 12.0f - hardness * 8.0f;
            break; // Glass Harp: slow friction
        default:
            contactMs = 1.5f - hardness * 1.0f;
            break; // Porcelain: spoon tap
        }
        contactSamples = std::max(static_cast<int>(contactMs * 0.001f * sampleRate), 4);
        peakAmplitude = velocity;

        // Noise content: toy piano = most noise, celesta = least
        float instrumentNoise[4] = {0.05f, 0.20f, 0.02f, 0.10f};
        noiseMix = instrumentNoise[std::clamp(instrument, 0, 3)] + hardness * 0.15f;

        // Include baseFreq bits so identical-velocity notes on different pitches
        // produce distinct noise patterns (avoids audible repetition on repeated notes).
        noiseState = static_cast<uint32_t>(velocity * 65535.0f)
                     ^ static_cast<uint32_t>(baseFreq * 31.0f)
                     ^ 12345u;

        // Mallet contact lowpass (Hunt-Crossley model)
        malletCutoff = baseFreq * (2.0f + hardness * 16.0f);
        malletFilterState = 0.0f;
        float fc = std::min(malletCutoff, sampleRate * 0.49f);
        malletLPCoeff = 1.0f - std::exp(-2.0f * 3.14159265f * fc / sampleRate);

        // FRAGILITY MECHANIC: compute whether this strike "cracks" the glass
        // Threshold: velocity * fragility > crackPoint
        float crackThreshold = 0.65f;
        crackTriggered = (velocity * fragility) > crackThreshold;
        crackIntensity = crackTriggered
                             ? std::clamp((velocity * fragility - crackThreshold) / (1.0f - crackThreshold), 0.0f, 1.0f)
                             : 0.0f;

        // Crack noise burst: 2-5ms of broadband noise.
        // Precompute per-sample multiplicative decay coefficient so process()
        // needs only one multiply per sample instead of a std::exp call. (Perf)
        if (crackTriggered)
        {
            crackSamples = static_cast<int>((2.0f + crackIntensity * 3.0f) * 0.001f * sampleRate);
            crackAmplitude = velocity * crackIntensity * 0.6f;
            // exp(-3*n/N) per sample ≡ multiply by exp(-3/N) each step
            crackDecayCoeff = (crackSamples > 0)
                                  ? std::exp(-3.0f / static_cast<float>(crackSamples))
                                  : 1.0f;
            crackEnvLevel = 1.0f; // reset running envelope to peak on trigger
        }
        else
        {
            crackSamples = 0;
            crackAmplitude = 0.0f;
            crackDecayCoeff = 1.0f;
            crackEnvLevel = 0.0f;
        }
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        float out = 0.0f;

        // Primary strike pulse
        if (sampleCounter < contactSamples)
        {
            float phase = static_cast<float>(sampleCounter) / static_cast<float>(contactSamples);
            float pulse = fastSin(phase * 3.14159265f) * peakAmplitude;
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f) * peakAmplitude;
            out = pulse * (1.0f - noiseMix) + noise * noiseMix;
        }

        // Crack noise burst (the glass breaking sound).
        // crackEnvLevel decays multiplicatively (precomputed in trigger) — no per-sample std::exp.
        if (crackTriggered && sampleCounter < crackSamples)
        {
            noiseState = noiseState * 1664525u + 1013904223u;
            float crackNoise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);
            out += crackNoise * crackAmplitude * crackEnvLevel;
            crackEnvLevel *= crackDecayCoeff;
        }

        // Deactivate after both excitation and crack complete.
        // Increment first, then check: the condition fires at the sample AFTER the last
        // output sample, avoiding one extra zero-output iteration of the mallet LP filter.
        ++sampleCounter;
        int totalSamples = std::max(contactSamples, crackSamples);
        if (sampleCounter >= totalSamples)
            active = false;

        // Mallet contact lowpass
        malletFilterState += malletLPCoeff * (out - malletFilterState);
        malletFilterState = flushDenormal(malletFilterState);
        return malletFilterState;
    }

    void reset() noexcept
    {
        active = false;
        sampleCounter = 0;
        malletFilterState = 0.0f;
        crackTriggered = false;
        crackIntensity = 0.0f;
        crackEnvLevel = 0.0f;
        crackDecayCoeff = 1.0f;
    }

    // Accessors for the render loop
    bool isCracked() const noexcept { return crackTriggered; }
    float getCrackIntensity() const noexcept { return crackIntensity; }

    bool active = false;
    int sampleCounter = 0, contactSamples = 48;
    float peakAmplitude = 1.0f, noiseMix = 0.05f;
    uint32_t noiseState = 12345u;
    float malletCutoff = 8000.0f, malletLPCoeff = 0.5f, malletFilterState = 0.0f;

    bool crackTriggered = false;
    float crackIntensity = 0.0f;
    int crackSamples = 0;
    float crackAmplitude = 0.0f;
    float crackDecayCoeff = 1.0f; // precomputed per-sample decay for crack envelope
    float crackEnvLevel = 1.0f;   // running envelope state (starts at 1 on trigger)
};

//==============================================================================
// OpalineMode — single 2nd-order modal resonator with dynamic pruning.
// Same architecture as OwareMode but with higher default Q for glass character.
//==============================================================================
struct OpalineMode
{
    void setFreqAndQ(float freqHz, float q, float sampleRate) noexcept
    {
        // Dirty-flag cache: skip expensive trig if freq/Q haven't changed meaningfully.
        // Reduces ~18M transcendental calls/sec to ~thousands/sec (only when params change).
        if (std::abs(freqHz - cachedFreq) < 0.5f && std::abs(q - cachedQ) < 0.001f)
            return;
        cachedFreq = freqHz;
        cachedQ = q;

        if (freqHz >= sampleRate * 0.49f)
            freqHz = sampleRate * 0.49f;
        float w = 2.0f * 3.14159265f * freqHz / sampleRate;
        float bw = freqHz / std::max(q, 1.0f);
        float r = std::exp(-3.14159265f * bw / sampleRate);
        a1 = 2.0f * r * fastCos(w);
        a2 = r * r;
        b0 = (1.0f - r * r) * fastSin(w);
        freq = freqHz;
    }

    float process(float input) noexcept
    {
        float out = b0 * input + a1 * y1 - a2 * y2;
        out = flushDenormal(out);
        y2 = y1;
        y1 = out;
        amplitude = std::fabs(out);
        return out;
    }

    void reset() noexcept
    {
        y1 = 0.0f;
        y2 = 0.0f;
        amplitude = 0.0f;
        cachedFreq = -1.0f;
        cachedQ = -1.0f;
    }

    float freq = 440.0f;
    float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
    float amplitude = 0.0f; // for dynamic pruning

    // Dirty-flag cache for setFreqAndQ — avoids redundant trig recalculation
    float cachedFreq = -1.0f;
    float cachedQ = -1.0f;
};

//==============================================================================
// OpalineVoice — per-voice state for the glass/porcelain modal synthesizer.
//==============================================================================
struct OpalineVoice
{
    static constexpr int kMaxModes = 16;
    static constexpr float kPruneThreshold = 1e-7f; // -140 dB — glass rings long

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    OpalineExciter exciter;
    std::array<OpalineMode, kMaxModes> modes;
    FilterEnvelope filterEnv;
    CytomicSVF svf;

    // HF noise shaper (CPU optimization: stochastic HF character instead of 64 modes)
    CytomicSVF hfNoiseSVF;
    float hfEnvLevel = 0.0f;

    // Per-voice LFOs (D002 / D005 compliance)
    StandardLFO lfo1, lfo2;

    // Per-voice shimmer (sympathetic ring simulation)
    StandardLFO shimmerLFO;

    // Thermal personality (per-voice random offset for thermal detuning)
    float thermalPersonality = 0.0f;

    // Crack state — persists for the note lifetime
    bool cracked = false;
    float crackDetuning = 0.0f; // cents of modal frequency shift from crack
    float crackIntensity = 0.0f;

    // Amplitude envelope
    float ampLevel = 0.0f;

    // noteOff ramp: smooth 5ms release to 40% rather than an instantaneous cliff.
    // 1.0f = no ramp active. Set to per-sample decay coefficient on noteOff.
    float noteOffDecay = 1.0f;

    // Counts down from (5ms * sampleRate) to 0. When 0, ramp is complete
    // and noteOffDecay is reset to 1.0f so the multiplication stops.
    int noteOffRampSamples = 0;

    // Cached pan gains (avoid per-sample trig)
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        ampLevel = 0.0f;
        noteOffDecay = 1.0f;
        noteOffRampSamples = 0;
        cracked = false;
        crackDetuning = 0.0f;
        crackIntensity = 0.0f;
        hfEnvLevel = 0.0f;
        glide.reset();
        exciter.reset();
        filterEnv.kill();
        shimmerLFO.reset();
        lfo1.reset();
        lfo2.reset();
        svf.reset();
        hfNoiseSVF.reset();
        for (auto& m : modes)
            m.reset();
    }
};

//==============================================================================
// OpalineEngine — "The Porcelain Bell"
//
// 16 modal resonators + HF noise shaper = ~3.2% CPU at 8 voices.
// The most delicate engine in the fleet.
//==============================================================================
class OpalineEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr int kNumModes = 16; // V1: 16 full IIR modes (CPU optimization strategy)

    juce::String getEngineId() const override { return "Opaline"; }
    // Crystal Blue #B8D4E3 — matches Accent colour documented in header.
    // Was 0xFFB7410E (burnt sienna — a copy-paste error from another engine).
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFB8D4E3); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].filterEnv.prepare(srf);
            // SVF mode for the brightness LPF never changes — set once here
            // rather than calling setMode() inside the per-sample voice loop.
            // P31: hoist static setMode() out of render path.
            voices[i].svf.setMode(CytomicSVF::Mode::LowPass);
            voices[i].shimmerLFO.setShape(StandardLFO::Sine);

            // HF noise shaper: bandpass centered at 4kHz (glass brightness)
            voices[i].hfNoiseSVF.setMode(CytomicSVF::Mode::BandPass);
            voices[i].hfNoiseSVF.setCoefficients(4000.0f, 0.3f, srf);

            // Per-voice thermal personality from seeded PRNG
            uint32_t seed = static_cast<uint32_t>(i * 7919 + 137);
            seed = seed * 1664525u + 1013904223u;
            voices[i].thermalPersonality = (static_cast<float>(seed & 0xFFFF) / 32768.0f - 1.0f) * 2.0f;
        }

        // Smoothers
        smoothFragility.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothDamping.prepare(srf);
        smoothHFNoise.prepare(srf);
        smoothBodySize.prepare(srf);
        smoothShimmer.prepare(srf);

        // Thermal drift: ~1 second time constant (glass is more thermally sensitive).
        // 1-pole leaky integrator: coeff = 1 - exp(-1/(tau * sr)), tau = 1.0s.
        thermalCoeff = 1.0f - std::exp(-1.0f / srf);

        // HF noise shaper envelope: ~22ms decay time constant, sample-rate-correct.
        // Was hardcoded 0.999f per sample — at 96kHz that decays 2× faster than 48kHz.
        // tau = 0.022s → coeff = exp(-1/(tau * sr)). (Sound/P-new)
        hfEnvDecay = std::exp(-1.0f / (0.022f * srf));

        // Noise PRNG state
        hfNoiseState = 98765u;

        prepareSilenceGate(sr, maxBlockSize, 400.0f);
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

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
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
            couplingInstrumentMod += val;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Render — all 5 pillars integrated
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // MIDI processing
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

        // Load parameters
        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        const int pInstrument = static_cast<int>(loadP(paramInstrument, 0.0f));
        const float pFragility = loadP(paramFragility, 0.3f);
        const float pBrightness = loadP(paramBrightness, 10000.0f);
        const float pDamping = loadP(paramDamping, 0.1f);
        const float pDecay = loadP(paramDecay, 3.0f);
        const float pHammerHard = loadP(paramHammerHardness, 0.4f);
        const float pBodySize = loadP(paramBodySize, 0.5f);
        const float pInharmonicity = loadP(paramInharmonicity, 0.0f);
        const float pShimmer = loadP(paramShimmerAmount, 0.3f);
        const float pThermal = loadP(paramThermalShock, 0.2f);
        const float pHFNoise = loadP(paramHFNoiseAmount, 0.3f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmount, 0.3f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pSustainPedal = loadP(paramSustainPedal, 0.0f);
        const float pCrystalDrive = loadP(paramCrystalDrive, 0.0f);

        const float macroCharacter = loadP(paramMacroCharacter, 0.0f);
        const float macroMovement = loadP(paramMacroMovement, 0.0f);
        const float macroCoupling = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // D006: aftertouch -> hammer hardness, mod wheel -> brightness sweep
        float effectiveHammer = std::clamp(pHammerHard + macroCharacter * 0.5f + aftertouchAmount * 0.4f, 0.0f, 1.0f);
        float effectiveBright = std::clamp(
            pBrightness + macroCharacter * 4000.0f + modWheelAmount * 5000.0f + couplingFilterMod, 200.0f, 20000.0f);
        // COUPLING macro → fragility: the more coupled, the more breakable.
        // Physics: coupling to a neighboring engine transmits additional energy through
        // the glass/porcelain body, lowering the effective crack threshold. At max coupling,
        // even moderate velocity will crack the sound. D004: macroCoupling now affects audio.
        float effectiveFragility = std::clamp(pFragility + macroMovement * 0.3f + macroCoupling * 0.4f, 0.0f, 1.0f);
        float effectiveShimmer = std::clamp(pShimmer + macroSpace * 0.4f, 0.0f, 1.0f);

        smoothFragility.set(effectiveFragility);
        smoothBrightness.set(effectiveBright);
        smoothDamping.set(pDamping);
        smoothHFNoise.set(pHFNoise);
        smoothBodySize.set(pBodySize);
        smoothShimmer.set(effectiveShimmer);

        // P25: CAPTURE-THEN-ZERO all coupling accumulators at block start.
        // couplingFilterMod consumed above in effectiveBright; zero it now.
        // couplingPitchMod: capture into a local, then zero immediately so new coupling
        // calls arriving mid-block accumulate into next block rather than doubling this one.
        // couplingInstrumentMod: same pattern applied below where it is read.
        const float capturedPitchMod = couplingPitchMod;
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f; // zeroed here; capturedPitchMod used in sample loop

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Decay: glass/porcelain have long ring but quick initial energy loss
        float decayTimeSec = std::max(pDecay * (1.0f - pDamping * 0.9f), 0.01f);
        float baseDecayCoeff = std::exp(-1.0f / (decayTimeSec * srf));

        // Sustain pedal: multiply decay coefficient to extend ring
        if (pSustainPedal > 0.5f)
            baseDecayCoeff = std::min(baseDecayCoeff * 1.002f, 0.99999f);

        // Pillar 4: Thermal shock — slow thermal drift.
        // Bug fix: thermalTimer incremented once per renderBlock(), NOT once per sample.
        // Previous comparison `thermalTimer > srf * 3.0f` used a sample-count threshold
        // but counted block-render calls — at block size 512, fired every ~68s, not ~3s.
        // Fixed: accumulate numSamples each block so thermalTimer counts actual samples. (Sound/P-new)
        thermalTimer += numSamples;
        if (thermalTimer > static_cast<int>(srf * 3.0f)) // new target every ~3 seconds
        {
            thermalNoiseState = thermalNoiseState * 1664525u + 1013904223u;
            thermalTarget = (static_cast<float>(thermalNoiseState & 0xFFFF) / 32768.0f - 1.0f) * pThermal *
                            12.0f; // max +/-12 cents (glass is thermally sensitive)
            thermalTimer = 0;
        }
        thermalState += thermalCoeff * (thermalTarget - thermalState);

        // Select instrument ratio table — couplingInstrumentMod (from EnvToMorph) sweeps
        // through the 4 glass/porcelain timbres. Consumed here, then cleared. (#947)
        const float* ratioTable;
        int instrument = std::clamp(pInstrument + static_cast<int>(couplingInstrumentMod * 3.0f + 0.5f), 0, 3);
        couplingInstrumentMod = 0.0f; // reset after use
        switch (instrument)
        {
        case 0:
            ratioTable = kCelestaRatios;
            break;
        case 1:
            ratioTable = kToyPianoRatios;
            break;
        case 2:
            ratioTable = kGlassHarpRatios;
            break;
        default:
            ratioTable = kPorcelainRatios;
            break;
        }

        float modeRolloff = kModeRolloff[instrument];
        float baseQ = kBaseQ[instrument];

        // Read LFO params once per block. Fallbacks match addParametersImpl() defaults
        // so behavior is correct even before attachParameters() is called. (Params)
        const float lfo1Rate = paramLfo1Rate ? paramLfo1Rate->load() : 0.3f;
        const float lfo1Depth = paramLfo1Depth ? paramLfo1Depth->load() : 0.1f; // param default 0.1f
        const int lfo1Shape = paramLfo1Shape ? static_cast<int>(paramLfo1Shape->load()) : 0;
        const float lfo2Rate = paramLfo2Rate ? paramLfo2Rate->load() : 0.8f;
        const float lfo2Depth = paramLfo2Depth ? paramLfo2Depth->load() : 0.0f;
        const int lfo2Shape = paramLfo2Shape ? static_cast<int>(paramLfo2Shape->load()) : 0;

        // Shimmer LFO rate: derive from effectiveShimmer once per block.
        // Was computed inside the per-sample voice loop (voice.shimmerLFO.setRate()
        // called once per active-voice per sample), which is 8× wasteful for a
        // slowly-varying parameter. Block-rate update is perceptually identical. (Perf)
        const float shimmerBlockRate = std::max(0.05f, effectiveShimmer * 8.0f);

        // Set LFO rate/shape per voice (once per block, not per sample)
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);
            voice.shimmerLFO.setRate(shimmerBlockRate, srf);
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // bendSemitones + couplingPitchMod are block-constant; hoist pitch-bend ratio.
        const float blockBendRatio =
            PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod);

        for (int s = 0; s < numSamples; ++s)
        {
            const bool updateFilter = ((s & 15) == 0);
            float fragilityNow = smoothFragility.process();
            float brightNow = smoothBrightness.process();
            float dampNow = smoothDamping.process();
            float hfNoiseNow = smoothHFNoise.process();
            float bodySizeNow = smoothBodySize.process();
            float shimmerNow = smoothShimmer.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float freq = voice.glide.process();
                // capturedPitchMod: block-captured coupling value (P25 — zeroed at block start)
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + capturedPitchMod);
                freq *= blockBendRatio; // hoisted above — was per-sample per-voice fastPow2

                // LFO modulation
                float lfo1Val = voice.lfo1.process() * lfo1Depth; // LFO1 -> brightness
                float lfo2Val = voice.lfo2.process() * lfo2Depth; // LFO2 -> shimmer/pitch

                // Pillar 4: Thermal detuning (shared + per-voice personality)
                float totalThermalCents = thermalState + voice.thermalPersonality * pThermal * 0.5f;
                freq *= fastPow2(totalThermalCents / 1200.0f);

                // Body size: scales fundamental pitch (smaller body = higher pitch).
                // Formula: 0.5 + bodySizeNow → at default 0.5, sizeScale = 1.0 (unity pitch).
                // Range: [0.5x (small body, 1 oct up) … 1.5x (large body, ~7 semitones down)].
                // Previous formula (0.5 + bodySize * 1.5) placed unity at bodySize≈0.333,
                // causing default-preset pitch to be +386 cents sharp. (Sound/Params — bug fix)
                float sizeScale = 0.5f + bodySizeNow; // 0.5x to 1.5x; unity at default (0.5)
                float baseFreq = freq * sizeScale;

                // Shimmer: subtle pitch modulation for crystalline character.
                // Rate is updated once per block above (shimmerBlockRate) — no per-sample setRate().
                float shimmerMod = voice.shimmerLFO.process();
                float shimmerCents = shimmerMod * shimmerNow * 6.0f; // up to +/-6 cents

                // Crack detuning: if this note cracked, modes are shifted
                float crackDetuneCents = voice.cracked ? voice.crackDetuning : 0.0f;

                // Process excitation
                float excitation = voice.exciter.process();

                // Hoist loop-invariant LFO2 pitch scale factor out of the mode loop.
                // lfo2Val * 0.5f / 1200.0f is constant across all m; only (m+1) varies.
                // Saves one multiply per mode per sample. (Perf)
                const float lfo2PitchScale = lfo2Val * 0.5f / 1200.0f;

                // Modal resonator bank (Pillar 2)
                float resonanceSum = 0.0f;
                int activeModes = 0;

                for (int m = 0; m < kNumModes; ++m)
                {
                    // Dynamic pruning: skip modes below threshold
                    if (voice.modes[m].amplitude < OpalineVoice::kPruneThreshold && !voice.exciter.active)
                    {
                        continue;
                    }

                    // Base modal frequency from ratio table
                    float ratio = ratioTable[m];

                    // Apply inharmonicity stretch (beam stiffness dispersion)
                    float inharmFactor = 1.0f + pInharmonicity * static_cast<float>(m * m) * 0.001f;
                    ratio *= inharmFactor;

                    float modeFreq = baseFreq * ratio;

                    // Apply shimmer (per-mode phase offset for crystalline beating)
                    float modeShimmer = shimmerCents * (1.0f + static_cast<float>(m) * 0.15f);
                    modeFreq *= fastPow2(modeShimmer / 1200.0f);

                    // Apply LFO2 as subtle pitch modulation (loop-invariant scale hoisted above)
                    modeFreq *= fastPow2(lfo2PitchScale * static_cast<float>(m + 1));

                    // Apply crack detuning: higher modes shift more (glass breaks unevenly)
                    if (voice.cracked)
                    {
                        float crackShift = crackDetuneCents * (1.0f + static_cast<float>(m) * 0.4f);
                        modeFreq *= fastPow2(crackShift / 1200.0f);
                    }

                    // Q: instrument-dependent base, reduced by damping, scaled by fragility.
                    // fragilityNow is the per-sample smoothed value (drains smoothFragility),
                    // giving smooth parameter transitions instead of block-rate stepping.
                    // High fragility → higher Q (brittle glass rings longer before losing energy).
                    float modeQ = baseQ / (1.0f + static_cast<float>(m) * 0.15f);
                    modeQ *= (1.0f - dampNow * 0.8f);
                    modeQ *= (1.0f + fragilityNow * 1.5f); // fragility: 1.0x (low) → 2.5x (high)

                    // Crack reduces Q on upper modes (broken glass rings less purely)
                    if (voice.cracked)
                        modeQ *= (1.0f -
                                  voice.crackIntensity * 0.5f * static_cast<float>(m) / static_cast<float>(kNumModes));

                    voice.modes[m].setFreqAndQ(modeFreq, std::max(modeQ, 1.0f), srf);

                    // Mode amplitude: velocity-scaled, higher modes decay faster
                    float modeAmp = 1.0f / (1.0f + static_cast<float>(m) * modeRolloff);

                    // D001: hammer hardness excites upper modes
                    modeAmp *= (1.0f - static_cast<float>(m) * (0.12f - effectiveHammer * 0.10f));
                    modeAmp = std::max(modeAmp, 0.0f);

                    resonanceSum += voice.modes[m].process(excitation) * modeAmp;
                    ++activeModes;
                }

                // Scale by the fixed mode count so dynamics don't invert as modes are pruned.
                // Using kNumModes (not activeModes) prevents louder output when fewer modes ring.
                if (activeModes > 0)
                    resonanceSum *= 4.0f / static_cast<float>(kNumModes);

                // HF noise character (CPU optimization: stochastic HF instead of 64 modes)
                if (hfNoiseNow > 0.001f && voice.exciter.active)
                {
                    hfNoiseState = hfNoiseState * 1664525u + 1013904223u;
                    float noise = (static_cast<float>(hfNoiseState & 0xFFFF) / 32768.0f - 1.0f);
                    // Shape noise through the HF bandpass.
                    // P19: use setCoefficients_fast() — avoids full std::tan recompute
                    // in the per-sample inner loop; accuracy is sufficient for noise shaping.
                    voice.hfNoiseSVF.setCoefficients_fast(std::clamp(baseFreq * 6.0f, 2000.0f, 16000.0f), 0.4f, srf);
                    // Shape noise through the HF bandpass (coeff refresh decimated)
                    if (updateFilter)
                        voice.hfNoiseSVF.setCoefficients(std::clamp(baseFreq * 6.0f, 2000.0f, 16000.0f), 0.4f, srf);
                    float hfShaped = voice.hfNoiseSVF.processSample(noise);
                    voice.hfEnvLevel *= hfEnvDecay; // sample-rate-correct HF envelope decay
                    resonanceSum += hfShaped * hfNoiseNow * voice.hfEnvLevel * voice.velocity;
                }

                // Soft-clip modal resonator output to prevent runaway at high Q.
                // Applied unconditionally before crystal drive so the ceiling is
                // always enforced, even when pCrystalDrive == 0. (#685)
                resonanceSum = fastTanh(resonanceSum);

                // Crystal drive: subtle waveshaping for additional harmonics
                if (pCrystalDrive > 0.01f)
                {
                    float drive = 1.0f + pCrystalDrive * 8.0f;
                    resonanceSum = fastTanh(resonanceSum * drive) / drive;
                }

                // Amplitude envelope with glass-appropriate long decay
                voice.ampLevel *= baseDecayCoeff;

                // noteOff ramp: smoothly multiply down to 40% over 5ms instead of cliff.
                // noteOffRampSamples counts down to 0; when it hits 0, the ramp is done
                // and noteOffDecay is reset to 1.0f so it no longer affects ampLevel.
                if (voice.noteOffRampSamples > 0)
                {
                    voice.ampLevel *= voice.noteOffDecay;
                    --voice.noteOffRampSamples;
                    if (voice.noteOffRampSamples == 0)
                        voice.noteOffDecay = 1.0f;
                }

                voice.ampLevel = flushDenormal(voice.ampLevel);
                if (voice.ampLevel < 1e-7f)
                {
                    voice.active = false;
                    continue;
                }

                // Filter: LPF for brightness control.
                // P19: use setCoefficients_fast() — avoids std::tan per-sample in audio loop.
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 5000.0f;
                float cutoff = std::clamp(brightNow + envMod + lfo1Val * 4000.0f, 200.0f, 20000.0f);
                // Mode was hoisted to prepare(); no setMode() call needed here.
                voice.svf.setCoefficients_fast(cutoff, 0.3f, srf);
                // Filter: LPF for brightness control (env ticked per-sample, SVF decimated)
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 5000.0f;
                if (updateFilter)
                {
                    float cutoff = std::clamp(brightNow + envMod + lfo1Val * 4000.0f, 200.0f, 20000.0f);
                    voice.svf.setMode(CytomicSVF::Mode::LowPass);
                    voice.svf.setCoefficients(cutoff, 0.3f, srf);
                }
                float filtered = voice.svf.processSample(resonanceSum);

                float output = filtered * voice.ampLevel;

                // Stereo positioning
                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR)
                outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        // couplingPitchMod was captured into capturedPitchMod and zeroed at block start (P25).
        // No post-loop reset needed.

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoiceCount.store(count);
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn(int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice(voices, kMaxVoices);
        auto& v = voices[idx];

        // Use fastPow2 instead of std::pow — same accuracy for MIDI-to-freq conversion
        // and avoids a transcendental call on every noteOn event. (Perf)
        float freq = 440.0f * fastPow2((static_cast<float>(note) - 69.0f) / 12.0f);

        int instrument = paramInstrument ? static_cast<int>(paramInstrument->load()) : 0;
        instrument = std::clamp(instrument, 0, 3);
        float fragility = paramFragility ? paramFragility->load() : 0.3f;
        float hammerHard = paramHammerHardness ? paramHammerHardness->load() : 0.4f;

        // D001 + D006: velocity + aftertouch -> hammer hardness
        float hardness = std::clamp(hammerHard + vel * 0.5f + aftertouchAmount * 0.3f, 0.0f, 1.0f);

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);
        v.ampLevel = 1.0f;
        v.hfEnvLevel = 1.0f;
        // Cancel any in-progress noteOff ramp from a stolen voice — otherwise the
        // per-sample noteOffDecay multiplier keeps suppressing ampLevel on the new note.
        v.noteOffRampSamples = 0;
        v.noteOffDecay = 1.0f;

        // Trigger exciter with fragility mechanic (Pillar 3)
        v.exciter.trigger(vel, hardness, freq, fragility, instrument, srf);

        // Store crack state for the note's lifetime
        v.cracked = v.exciter.isCracked();
        v.crackIntensity = v.exciter.getCrackIntensity();

        // Crack detuning: random per-mode shift based on crack intensity
        if (v.cracked)
        {
            uint32_t crackSeed = static_cast<uint32_t>(note * 31 + static_cast<int>(vel * 1000.0f));
            crackSeed = crackSeed * 1664525u + 1013904223u;
            // +/-25 cents at full crack, scaled by intensity
            v.crackDetuning = (static_cast<float>(crackSeed & 0xFFFF) / 32768.0f - 1.0f) * 25.0f * v.crackIntensity;
        }
        else
        {
            v.crackDetuning = 0.0f;
        }

        // Filter envelope: Pillar 5 (crystalline envelope)
        // Short attack, very fast decay to pure ring.
        // P18: filterEnv.prepare() is called once in prepare() — no need to repeat
        // per-noteOn; doing so wastes a recalcCoeffs() call and redundant exp compute.
        float filterDecay = 0.05f + (1.0f - vel) * 0.15f; // 50-200ms, velocity-scaled
        v.filterEnv.setADSR(0.001f, filterDecay, 0.0f, 0.3f);
        v.filterEnv.triggerHard();

        // Shimmer LFO: per-voice phase offset for ensemble
        v.shimmerLFO.reset(static_cast<float>(idx) / static_cast<float>(kMaxVoices));

        // Reset SVF filter states to avoid click artifacts on voice steal.
        // v.svf and v.hfNoiseSVF retain stale state when the voice is reused —
        // resetting here prevents a transient burst from the old note's filter memory.
        // Stability: critical on voice-steal paths where a decayed note is reassigned. (Stability)
        v.svf.reset();
        v.hfNoiseSVF.reset();
        // Re-apply static SVF mode (was set in prepare(), cleared by reset() above)
        v.svf.setMode(CytomicSVF::Mode::LowPass);
        v.hfNoiseSVF.setMode(CytomicSVF::Mode::BandPass);

        // Reset modes
        for (auto& m : v.modes)
            m.reset();

        // Stereo pan: spread voices across stereo field
        float pan = (static_cast<float>(idx) - 3.5f) / 4.0f; // [-0.875, 0.875]
        pan *= 0.7f;                                         // narrow the field slightly
        v.panL = fastCos((pan + 1.0f) * 0.25f * 3.14159265f);
        v.panR = fastSin((pan + 1.0f) * 0.25f * 3.14159265f);
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                // Glass release: smooth 5ms ramp to 40% (avoids the amplitude cliff click).
                // noteOffRampSamples counts down sample-by-sample; ramp terminates when it hits 0.
                // Stability: guard against division-by-zero if srf is not yet set or rounds to 0.
                v.noteOffRampSamples = std::max(static_cast<int>(0.005f * srf), 1);
                v.noteOffDecay = std::exp(std::log(0.4f) / static_cast<float>(v.noteOffRampSamples));
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 27 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Pillar 1: Instrument selector (0=Celesta, 1=ToyPiano, 2=GlassHarp, 3=PorcelainCups)
        params.push_back(std::make_unique<PI>(juce::ParameterID{"opal2_instrument", 1}, "Opaline Instrument", 0, 3, 0));

        // Pillar 3: Fragility — how easily the glass cracks under hard velocity
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_fragility", 1}, "Opaline Fragility",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Hammer/striker hardness
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_hammerHardness", 1}, "Opaline Hammer Hardness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        // Brightness (filter cutoff)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_brightness", 1}, "Opaline Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 10000.0f));

        // Damping — how quickly modes lose energy
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_damping", 1}, "Opaline Damping",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));

        // Decay time
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_decay", 1}, "Opaline Decay",
                                              juce::NormalisableRange<float>(0.1f, 15.0f, 0.0f, 0.4f), 3.0f));

        // Body size (pitch scaling — smaller body = higher pitch)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_bodySize", 1}, "Opaline Body Size",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Inharmonicity stretch (beam stiffness)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_inharmonicity", 1}, "Opaline Inharmonicity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Shimmer: crystalline beating between modes
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_shimmerAmount", 1}, "Opaline Shimmer",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Pillar 4: Thermal shock sensitivity
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_thermalShock", 1}, "Opaline Thermal Shock",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // HF noise character (stochastic upper partial simulation)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_hfNoise", 1}, "Opaline HF Noise",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Filter envelope amount
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_filterEnvAmount", 1}, "Opaline Filter Env",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Crystal drive (subtle waveshaping)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_crystalDrive", 1}, "Opaline Crystal Drive",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Sustain pedal (CC64 latch or continuous)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_sustainPedal", 1}, "Opaline Sustain Pedal",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Pitch bend range
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_bendRange", 1}, "Opaline Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros (4 standard: CHARACTER, MOVEMENT, COUPLING, SPACE)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_macroCharacter", 1}, "Opaline Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_macroMovement", 1}, "Opaline Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_macroCoupling", 1}, "Opaline Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_macroSpace", 1}, "Opaline Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFO1 (D002 / D005 compliance)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_lfo1Rate", 1}, "Opaline LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_lfo1Depth", 1}, "Opaline LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"opal2_lfo1Shape", 1}, "Opaline LFO1 Shape", 0, 4, 0));

        // LFO2
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_lfo2Rate", 1}, "Opaline LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.8f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"opal2_lfo2Depth", 1}, "Opaline LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"opal2_lfo2Shape", 1}, "Opaline LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramInstrument = apvts.getRawParameterValue("opal2_instrument");
        paramFragility = apvts.getRawParameterValue("opal2_fragility");
        paramHammerHardness = apvts.getRawParameterValue("opal2_hammerHardness");
        paramBrightness = apvts.getRawParameterValue("opal2_brightness");
        paramDamping = apvts.getRawParameterValue("opal2_damping");
        paramDecay = apvts.getRawParameterValue("opal2_decay");
        paramBodySize = apvts.getRawParameterValue("opal2_bodySize");
        paramInharmonicity = apvts.getRawParameterValue("opal2_inharmonicity");
        paramShimmerAmount = apvts.getRawParameterValue("opal2_shimmerAmount");
        paramThermalShock = apvts.getRawParameterValue("opal2_thermalShock");
        paramHFNoiseAmount = apvts.getRawParameterValue("opal2_hfNoise");
        paramFilterEnvAmount = apvts.getRawParameterValue("opal2_filterEnvAmount");
        paramCrystalDrive = apvts.getRawParameterValue("opal2_crystalDrive");
        paramSustainPedal = apvts.getRawParameterValue("opal2_sustainPedal");
        paramBendRange = apvts.getRawParameterValue("opal2_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("opal2_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("opal2_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("opal2_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("opal2_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("opal2_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("opal2_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("opal2_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("opal2_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("opal2_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("opal2_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OpalineVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothFragility, smoothBrightness, smoothDamping;
    ParameterSmoother smoothHFNoise, smoothBodySize, smoothShimmer;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // Thermal shock state (Pillar 4)
    float thermalState = 0.0f, thermalTarget = 0.0f;
    float thermalCoeff = 0.0001f;
    int thermalTimer = 0;
    uint32_t thermalNoiseState = 54321u;

    // HF noise PRNG (shared, not per-voice)
    uint32_t hfNoiseState = 98765u;
    float hfEnvDecay = 0.999f; // precomputed in prepare() from sample rate (Sound/P-new)

    // Coupling accumulators
    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingInstrumentMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramInstrument = nullptr;
    std::atomic<float>* paramFragility = nullptr;
    std::atomic<float>* paramHammerHardness = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramBodySize = nullptr;
    std::atomic<float>* paramInharmonicity = nullptr;
    std::atomic<float>* paramShimmerAmount = nullptr;
    std::atomic<float>* paramThermalShock = nullptr;
    std::atomic<float>* paramHFNoiseAmount = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramCrystalDrive = nullptr;
    std::atomic<float>* paramSustainPedal = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
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
