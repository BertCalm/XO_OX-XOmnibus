#pragma once
//==============================================================================
//
//  OvenEngine.h — XOven | "The Cast Iron Concert Grand"
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOven is the 9-foot concert grand piano cast entirely in iron — a
//      Steinway D forged not from spruce and maple but from the same metal
//      as Dutch ovens and engine blocks. It sits in an empty hall at midnight,
//      massive and dark, absorbing hammer energy into its enormous thermal
//      mass and releasing it slowly as warm, sustaining resonance. Nothing
//      escapes quickly. Nothing is wasted. The cast iron traps everything.
//
//  ENGINE CONCEPT:
//      Modal synthesis piano where cast iron material physics determines every
//      sonic property. 16 IIR modal resonators per voice derive their
//      frequencies, Q factors, and decay rates from cast iron's mechanical
//      impedance (Z = rho * c = 7200 * 5100 = 36.72 MRayl). High impedance
//      mismatch with strings means most energy reflects back into the body,
//      creating massive sustain and dark warmth. HF noise fill above the
//      mode ceiling completes the spectral envelope.
//
//  MATERIAL PHYSICS:
//      Cast iron: rho = 7200 kg/m^3, c = 5100 m/s, Z = 36.72 MRayl
//      Young's modulus E = 170 GPa, internal loss factor eta ~= 0.003
//      Thermal coefficient: E(T) = E0 * (1 - 0.0003*T) => ~0.8 cents/10 deg C
//
//  REFERENCES:
//      - Fletcher & Rossing (1998), "The Physics of Musical Instruments"
//      - Chaigne & Kergomard (2016), "Acoustics of Musical Instruments"
//      - Bilbao (2009), "Numerical Sound Synthesis"
//      - Hunt & Crossley (1975), "Coefficient of Restitution"
//      - Audsley (1905), "The Art of Organ Building" (spectral envelopes)
//
//  CPU BUDGET:
//      16 modes * 5ns * 8 voices = 640 ns/sample
//      1 SVF noise shaper * 4ns * 8 voices = 32 ns/sample
//      12 sympathetic resonators * 5ns = 60 ns/sample (shared, not per-voice)
//      Total: ~732 ns/sample = ~3.2% CPU at 512-sample block
//      (Decision K1 + K2 from kitchen-cpu-optimization-strategy.md)
//
//  Accent: Cast Iron Black #2C2C2C
//  Parameter prefix: oven_
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

namespace xolokun {

//==============================================================================
// Cast Iron Piano Modal Ratios — from Chaigne & Kergomard (2016) Table 6.2,
// adapted for cast iron body resonance. Concert grand body modes follow a
// quasi-harmonic series modified by plate stiffness dispersion:
//   f_n = f_1 * n * sqrt(1 + B*n^2)   where B is the inharmonicity coefficient
//
// For cast iron with B ~= 0.0004 (high stiffness, high density):
//==============================================================================
static constexpr float kCastIronModeRatios[16] = {
    1.000f,   // fundamental body mode
    2.005f,   // ~octave, slight stretch from stiffness
    3.018f,   // 5th above octave
    4.042f,   // double octave
    5.079f,   // major 3rd above double octave
    6.131f,   // ~5th above double octave
    7.200f,   // natural 7th
    8.289f,   // triple octave (stretched)
    9.401f,   // above triple octave
    10.539f,  // major 3rd above triple octave
    11.705f,  // stretched partial
    12.903f,  // quadruple octave region
    14.136f,  // high partial, increasingly inharmonic
    15.408f,  // high metallic
    16.723f,  // near upper ceiling
    18.085f   // highest tracked mode — above this, noise fill takes over
};

// Audsley spectral envelope: amplitude weighting per mode for concert grand.
// Fundamental strongest, 1/n rolloff with slight boost at modes 3-5 (formant).
static constexpr float kAudsleyAmplitudes[16] = {
    1.000f, 0.620f, 0.550f, 0.480f, 0.400f, 0.310f, 0.240f, 0.185f,
    0.140f, 0.105f, 0.078f, 0.058f, 0.042f, 0.030f, 0.021f, 0.015f
};

//==============================================================================
// OvenModalResonator — second-order IIR resonator with dynamic pruning.
// Each represents one body mode of the cast iron piano frame.
// H(z) = b0 / (1 - a1*z^-1 - a2*z^-2)  where r = exp(-pi*bw/sr)
//==============================================================================
struct OvenModalResonator
{
    // CPU FIX: dirty-flag cache — setFreqAndDecay() only recomputes trig/exp when freq or Q changes.
    // Was called every sample per mode per voice (~256 fastSin + 256 fastCos + 256 fastExp per sample
    // at 8v×16m×3 ops). Q modulates slowly (via LFO2 at ±0.2), so the threshold guards correctly.
    void setFreqAndDecay (float freqHz, float q, float sampleRate) noexcept
    {
        if (freqHz >= sampleRate * 0.49f) freqHz = sampleRate * 0.49f;
        if (freqHz < 20.0f) freqHz = 20.0f;
        // Early-out: skip expensive trig/exp if inputs haven't changed meaningfully
        if (std::fabs (freqHz - freq) < 0.01f && std::fabs (q - cachedQ) < 0.5f)
            return;
        float w = 2.0f * 3.14159265f * freqHz / sampleRate;
        float bw = freqHz / std::max (q, 1.0f);
        float r = std::exp (-3.14159265f * bw / sampleRate);
        cosW = fastCos (w);
        a1 = 2.0f * r * cosW;
        a2 = r * r;
        b0 = (1.0f - r * r) * fastSin (w);
        freq = freqHz;
        cachedQ = q;
    }

    float process (float input) noexcept
    {
        float out = b0 * input + a1 * y1 - a2 * y2;
        out = flushDenormal (out);
        y2 = y1;
        y1 = out;
        amplitude = std::fabs (out);
        return out;
    }

    void reset() noexcept { y1 = 0.0f; y2 = 0.0f; amplitude = 0.0f; freq = -1.0f; cachedQ = -1.0f; }

    float freq = -1.0f;     // -1 forces recompute on first call
    float cachedQ = -1.0f;
    float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float cosW = 1.0f;
    float y1 = 0.0f, y2 = 0.0f;
    float amplitude = 0.0f;  // for dynamic pruning + sympathetic coupling
};

//==============================================================================
// OvenHammerModel — Hunt-Crossley nonlinear contact model (1975).
// Generates the hammer excitation impulse. Velocity controls hardness AND
// contact time, implementing the Maillard reaction analogy:
//   F = k * delta^p + c * delta^p * delta_dot
// Simplified for DSP: half-sine pulse with velocity-dependent width and
// spectral content (noise mix for hard hammer = char/crust of the sear).
//==============================================================================
struct OvenHammerModel
{
    void trigger (float velocity, float hardness, float baseFreq, float sampleRate) noexcept
    {
        active = true;
        sampleCounter = 0;

        // Hunt-Crossley: harder hammer = shorter contact = more HF excited
        // Cast iron concert grand: contact time 1-8ms depending on velocity
        // Soft felt (low vel): 6-8ms, rounded. Hard hammer (high vel): 1-2ms, sharp.
        float effectiveHardness = std::clamp (hardness + velocity * 0.5f, 0.0f, 1.0f);
        float contactMs = 8.0f - effectiveHardness * 7.0f;  // 8ms soft → 1ms hard
        contactSamples = std::max (static_cast<int> (contactMs * 0.001f * sampleRate), 4);

        peakAmplitude = velocity;

        // Noise mix = "Maillard char": hard strikes add noise content
        // Soft strikes are pure sine pulse (felt hammer), hard strikes add crackle
        noiseMix = effectiveHardness * effectiveHardness * 0.3f;
        noiseState = static_cast<uint32_t> (velocity * 65535.0f) + 54321u;

        // Spectral shaping: mallet contact lowpass
        // Soft hammer only excites fundamentals, hard hammer excites all modes
        float cutoffHz = baseFreq * (2.0f + effectiveHardness * 30.0f);  // 2x to 32x fundamental
        cutoffHz = std::min (cutoffHz, sampleRate * 0.49f);
        lpCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * cutoffHz / sampleRate);
        lpState = 0.0f;

        // Secondary micro-rebound for cast iron (slow body = gentle bounce at 20-30ms)
        // Cast iron's massive body has a slow rebound — the initial impulse spreads
        // slowly, so there's a subtle secondary peak
        reboundActive = (velocity > 0.3f);
        reboundSample = static_cast<int> (sampleRate * (0.020f + (1.0f - effectiveHardness) * 0.015f));
        reboundAmp = peakAmplitude * 0.15f * (1.0f - effectiveHardness * 0.5f);
    }

    float process() noexcept
    {
        if (!active) return 0.0f;

        float out = 0.0f;

        // Primary strike: half-sine pulse
        if (sampleCounter < contactSamples)
        {
            float phase = static_cast<float> (sampleCounter) / static_cast<float> (contactSamples);
            float pulse = std::sin (phase * 3.14159265f) * peakAmplitude;

            // Noise component (Maillard "char")
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f) * peakAmplitude;

            out = pulse * (1.0f - noiseMix) + noise * noiseMix;
        }

        // Micro-rebound (cast iron thermal mass = slow body reflection)
        if (reboundActive && sampleCounter >= reboundSample
            && sampleCounter < reboundSample + contactSamples / 2)
        {
            int rebPhase = sampleCounter - reboundSample;
            float phase = static_cast<float> (rebPhase) / static_cast<float> (contactSamples / 2);
            out += std::sin (phase * 3.14159265f) * reboundAmp;
        }

        // End condition
        if (sampleCounter >= reboundSample + contactSamples)
            active = false;

        ++sampleCounter;

        // Spectral lowpass: soft hammer = low-passed, hard hammer = open
        lpState += lpCoeff * (out - lpState);
        return lpState;
    }

    void reset() noexcept { active = false; sampleCounter = 0; lpState = 0.0f; }

    bool active = false;
    int sampleCounter = 0, contactSamples = 48;
    float peakAmplitude = 1.0f, noiseMix = 0.0f;
    uint32_t noiseState = 54321u;
    float lpCoeff = 0.5f, lpState = 0.0f;
    bool reboundActive = false;
    int reboundSample = 1000;
    float reboundAmp = 0.0f;
};

//==============================================================================
// OvenHFNoiseFill — Shaped noise above the modal ceiling.
// Per the CPU strategy: "above the 16th mode, shaped noise fill matches
// the spectral slope." One CytomicSVF per voice, negligible cost.
// The noise is triggered by the hammer and decays with the note.
//==============================================================================
struct OvenHFNoiseFill
{
    void trigger (float velocity, float brightness) noexcept
    {
        level = velocity * 0.08f;  // subtle — this is fill, not the main event
        decayCoeff = 0.9999f;      // long tail matching cast iron sustain
        noiseState = static_cast<uint32_t> (velocity * 12345.0f) + 67890u;
    }

    float process (float cutoffHz, float sampleRate) noexcept
    {
        if (level < 1e-7f) return 0.0f;

        // Generate white noise
        noiseState = noiseState * 1664525u + 1013904223u;
        float noise = static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f;

        // Shape with bandpass: above the modal ceiling but below brightness cutoff
        svf.setMode (CytomicSVF::Mode::BandPass);
        svf.setCoefficients (std::min (cutoffHz, sampleRate * 0.49f), 0.3f, sampleRate);
        float shaped = svf.processSample (noise) * level;

        level *= decayCoeff;
        level = flushDenormal (level);

        return shaped;
    }

    void reset() noexcept { level = 0.0f; svf.reset(); }

    float level = 0.0f;
    float decayCoeff = 0.9999f;
    uint32_t noiseState = 67890u;
    CytomicSVF svf;
};

//==============================================================================
// OvenVoice — single polyphonic voice for the Cast Iron Concert Grand.
//==============================================================================
struct OvenVoice
{
    static constexpr int kNumModes = 16;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;
    bool sustained = false;      // held by damper pedal
    bool noteHeld = false;       // key physically held

    GlideProcessor glide;
    OvenHammerModel hammer;
    std::array<OvenModalResonator, kNumModes> modes;
    OvenHFNoiseFill hfFill;
    FilterEnvelope filterEnv;
    FilterEnvelope ampEnv;       // amplitude envelope with cast iron character
    CytomicSVF outputFilter;     // main voice filter

    // D002: per-voice LFOs
    StandardLFO lfo1, lfo2;

    // Thermal personality: per-voice fixed detuning offset (cast iron non-uniformity)
    float thermalPersonality = 0.0f;

    // Bloom envelope: cast iron thermal mass metaphor — optional slow attack bloom
    float bloomLevel = 0.0f;
    float bloomTarget = 0.0f;
    float bloomCoeff = 0.0f;

    // Cached stereo pan
    float panL = 0.707f, panR = 0.707f;

    // Amp level (exponential decay for cast iron sustain)
    float ampLevel = 0.0f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        ampLevel = 0.0f;
        bloomLevel = 0.0f;
        sustained = false;
        noteHeld = false;
        glide.reset();
        hammer.reset();
        hfFill.reset();
        filterEnv.kill();
        ampEnv.kill();
        outputFilter.reset();
        lfo1.reset();
        lfo2.reset();
        for (auto& m : modes) m.reset();
    }
};

//==============================================================================
// OvenSympatheticNetwork — Shared post-mix sympathetic string resonator bank.
// Decision K2: NOT per-voice. One set of 12 resonators driven by the mix bus.
// Models the un-damped strings resonating sympathetically when the damper
// pedal is raised. Responds to harmonically related frequencies.
//==============================================================================
struct OvenSympatheticNetwork
{
    static constexpr int kMaxStrings = 12;

    struct SymString
    {
        OvenModalResonator resonator;
        float targetFreq = 0.0f;
        bool active = false;
    };

    std::array<SymString, kMaxStrings> strings;
    float outputLevel = 0.0f;

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        for (auto& s : strings)
        {
            s.resonator.reset();
            s.active = false;
        }
        outputLevel = 0.0f;
    }

    // Update which strings are active based on currently sounding notes.
    // Called on note-on/off. Finds the 12 most harmonically relevant strings.
    void updateActiveStrings (const int* activeNotes, int numActive, float sampleRate) noexcept
    {
        int stringIdx = 0;

        for (int n = 0; n < numActive && stringIdx < kMaxStrings; ++n)
        {
            float noteFreq = 440.0f * std::pow (2.0f, (static_cast<float> (activeNotes[n]) - 69.0f) / 12.0f);

            // Add sympathetic strings at harmonic intervals: octave, 5th, double octave
            float harmonics[] = { noteFreq * 2.0f, noteFreq * 3.0f, noteFreq * 0.5f };
            for (float hFreq : harmonics)
            {
                if (stringIdx >= kMaxStrings) break;
                if (hFreq < 20.0f || hFreq > 8000.0f) continue;

                strings[stringIdx].resonator.setFreqAndDecay (hFreq, 200.0f, sampleRate);
                strings[stringIdx].targetFreq = hFreq;
                strings[stringIdx].active = true;
                ++stringIdx;
            }
        }

        // Deactivate unused strings
        for (int i = stringIdx; i < kMaxStrings; ++i)
            strings[i].active = false;
    }

    // Process: feed the mix bus through the sympathetic resonators
    float process (float input, float depth) noexcept
    {
        if (depth < 0.001f) return 0.0f;

        float out = 0.0f;
        for (auto& s : strings)
        {
            if (!s.active) continue;
            out += s.resonator.process (input * 0.05f);  // low drive level
        }
        outputLevel = out * depth * 0.6f;  // cast iron: 0.6 sympathetic scaling
        return outputLevel;
    }

    void reset() noexcept
    {
        for (auto& s : strings)
        {
            s.resonator.reset();
            s.active = false;
        }
        outputLevel = 0.0f;
    }

    float sr = 48000.0f;
};

//==============================================================================
// OvenEngine — "The Cast Iron Concert Grand"
// Kitchen Collection Engine #1 | First of the KITCHEN Quad
//==============================================================================
class OvenEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr int kNumModes = OvenVoice::kNumModes;  // 16

    juce::String getEngineId() const override { return "Oven"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF2C2C2C); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].filterEnv.prepare (srf);
            voices[i].ampEnv.prepare (srf);

            // Per-voice thermal personality: cast iron bodies are never perfectly uniform
            // Seeded PRNG gives +-3 cents variance per voice
            uint32_t seed = static_cast<uint32_t> (i * 7919 + 42);
            seed = seed * 1664525u + 1013904223u;
            voices[i].thermalPersonality = (static_cast<float> (seed & 0xFFFF) / 32768.0f - 1.0f) * 3.0f;
        }

        sympatheticNetwork.prepare (srf);

        smoothBrightness.prepare (srf);
        smoothHardness.prepare (srf);
        smoothDensity.prepare (srf);
        smoothTemperature.prepare (srf);
        smoothSympathetic.prepare (srf);
        smoothBodyResonance.prepare (srf);
        smoothBloom.prepare (srf);

        // Thermal drift: ~4 second time constant for cast iron's massive thermal mass
        thermalCoeff = 1.0f - std::exp (-1.0f / (4.0f * srf));

        // SilenceGate: 500ms hold for piano sustain tails
        prepareSilenceGate (sr, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        sympatheticNetwork.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        damperPedal = false;
        thermalState = 0.0f;
        thermalTarget = 0.0f;
    }

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        (void) sampleIndex;
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                            const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0) return;
        float val = buf[numSamples - 1] * amount;
        switch (type) {
            case CouplingType::AmpToFilter:   couplingFilterMod += val * 2000.0f; break;
            case CouplingType::LFOToPitch:    couplingPitchMod += val * 2.0f; break;
            case CouplingType::AmpToPitch:    couplingPitchMod += val; break;
            case CouplingType::EnvToMorph:    couplingBodyMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render — Cast Iron Concert Grand modal synthesis
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Parse MIDI
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            else if (msg.isChannelPressure())
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController())
            {
                int cc = msg.getControllerNumber();
                float ccVal = msg.getControllerValue() / 127.0f;
                if (cc == 1)        modWheelAmount = ccVal;             // mod wheel
                else if (cc == 64)  handleDamperPedal (ccVal >= 0.5f);  // damper/sustain
            }
        }

        if (isSilenceGateBypassed())
        {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        // Snapshot parameters (once per block)
        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const float pHardness       = loadP (paramHardness, 0.4f);
        const float pBrightness     = loadP (paramBrightness, 3000.0f);
        const float pBodyResonance  = loadP (paramBodyResonance, 0.6f);
        const float pDensity        = loadP (paramDensity, 0.7f);
        const float pTemperature    = loadP (paramTemperature, 0.5f);
        const float pSympathetic    = loadP (paramSympathetic, 0.4f);
        const float pBloomTime      = loadP (paramBloomTime, 0.0f);
        const float pSustainTime    = loadP (paramSustainTime, 4.0f);
        const float pFilterEnvAmt   = loadP (paramFilterEnvAmt, 0.4f);
        const float pBendRange      = loadP (paramBendRange, 2.0f);
        const float pHFAmount       = loadP (paramHFAmount, 0.5f);

        // Amp envelope params
        const float pAmpAttack      = loadP (paramAmpAttack, 0.001f);
        const float pAmpDecay       = loadP (paramAmpDecay, 2.0f);
        const float pAmpSustain     = loadP (paramAmpSustain, 0.3f);
        const float pAmpRelease     = loadP (paramAmpRelease, 1.5f);

        // Macros: CHARACTER, MOVEMENT, COUPLING, SPACE
        const float macroCharacter  = loadP (paramMacroCharacter, 0.0f);
        const float macroMovement   = loadP (paramMacroMovement, 0.0f);
        const float macroCoupling   = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace      = loadP (paramMacroSpace, 0.0f);

        // LFO params — macroMovement boosts both LFO depths (D004: movement = more modulation)
        const float lfo1Rate  = loadP (paramLfo1Rate, 0.3f);
        const float lfo1Depth = std::clamp (loadP (paramLfo1Depth, 0.0f) + macroMovement * 0.3f, 0.0f, 1.0f);
        const int   lfo1Shape = paramLfo1Shape ? static_cast<int> (paramLfo1Shape->load()) : 0;
        const float lfo2Rate  = loadP (paramLfo2Rate, 0.8f);
        const float lfo2Depth = std::clamp (loadP (paramLfo2Depth, 0.0f) + macroMovement * 0.2f, 0.0f, 1.0f);
        const int   lfo2Shape = paramLfo2Shape ? static_cast<int> (paramLfo2Shape->load()) : 0;

        // Apply macros and expression
        // D006: mod wheel -> brightness (open up the spectral content)
        float effectiveBrightness = std::clamp (
            pBrightness + macroCharacter * 6000.0f + modWheelAmount * 4000.0f + couplingFilterMod,
            200.0f, 16000.0f);
        // D006: aftertouch -> body resonance and sustain
        float effectiveBodyRes = std::clamp (
            pBodyResonance + macroSpace * 0.3f + aftertouchAmount * 0.3f + couplingBodyMod,
            0.0f, 1.0f);
        float effectiveHardness = std::clamp (
            pHardness + macroCharacter * 0.3f, 0.0f, 1.0f);
        float effectiveSympathetic = std::clamp (
            pSympathetic + macroCoupling * 0.4f, 0.0f, 1.0f);
        // Damper pedal controls sympathetic engagement
        float sympAmount = damperPedal ? effectiveSympathetic : effectiveSympathetic * 0.1f;

        smoothBrightness.set (effectiveBrightness);
        smoothHardness.set (effectiveHardness);
        smoothDensity.set (pDensity);
        smoothTemperature.set (pTemperature);
        smoothSympathetic.set (sympAmount);
        smoothBodyResonance.set (effectiveBodyRes);
        smoothBloom.set (pBloomTime);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingBodyMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Thermal drift: cast iron's temperature slowly shifts tuning
        // ~0.8 cents per 10 degrees C of temperature offset
        thermalTimer++;
        if (thermalTimer > static_cast<int> (srf * 6.0f))  // new target every ~6 seconds (slow thermal mass)
        {
            thermalNoiseState = thermalNoiseState * 1664525u + 1013904223u;
            thermalTarget = (static_cast<float> (thermalNoiseState & 0xFFFF) / 32768.0f - 1.0f)
                          * pTemperature * 5.0f;  // max +-5 cents (cast iron is MASSIVE)
            thermalTimer = 0;
        }
        thermalState += thermalCoeff * (thermalTarget - thermalState);

        // Density controls Q factor: high density = high impedance = energy trapped = high Q
        // Cast iron range: Q 150-600 (very high — cast iron traps energy)
        float densityNow = smoothDensity.get();

        // Sustain time modulates amp envelope decay
        // Cast iron: long sustain (high impedance = energy can't escape)
        float sustainScale = std::max (pSustainTime * (0.5f + densityNow * 1.5f) / 4.0f, 0.1f);
        float scaledDecay = pAmpDecay * sustainScale;
        float scaledRelease = pAmpRelease * (0.5f + sustainScale * 0.5f);

        // ADVERSARIAL COUPLING PREP STUBS — Kitchen Quad V2 targets.
        // competition: cast iron's massive acoustic impedance (36.72 MRayl) under duress.
        //   At max, output saturation = cast iron piano suppressing smaller instruments.
        //   V2 target: cross-engine amplitude competition (XOven suppresses weaker bodies).
        // couplingResonance: sympathetic drive boost when coupled to another Kitchen engine.
        //   V2 target: impedance-transmission coupling (T = 4*Z1*Z2/(Z1+Z2)^2) per engine pair.
        // Both parameters produce audible DSP output now; behaviour will deepen in V2.
        float competitionLevel = loadP (paramCompetition, 0.0f);
        float couplingResLevel = loadP (paramCouplingRes, 0.0f);

        // Apply LFO rate/shape once per block
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);

            // Update amp envelope ADSR per block — sustainTime scales decay
            voice.ampEnv.setADSR (pAmpAttack, scaledDecay, pAmpSustain, scaledRelease);
        }

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float brightNow = smoothBrightness.process();
            float hardNow   = smoothHardness.process();
            float densNow   = smoothDensity.process();
            float tempNow   = smoothTemperature.process();
            float sympNow   = smoothSympathetic.process();
            float bodyRNow  = smoothBodyResonance.process();
            float bloomNow  = smoothBloom.process();

            float mixL = 0.0f, mixR = 0.0f;
            float sympatheticDrive = 0.0f;  // accumulate for shared sympathetic network

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                // D002: LFO modulation
                float lfo1Val = voice.lfo1.process() * lfo1Depth;  // LFO1 -> brightness
                float lfo2Val = voice.lfo2.process() * lfo2Depth;  // LFO2 -> body resonance

                // Apply thermal drift: shared + per-voice personality
                float totalThermalCents = thermalState + voice.thermalPersonality * tempNow * 0.3f;
                freq *= fastPow2 (totalThermalCents / 1200.0f);

                // Bloom: cast iron thermal mass metaphor — slow attack envelope
                // bloomNow 0.0 = instant attack, 1.0 = 200ms bloom
                if (bloomNow > 0.01f)
                {
                    voice.bloomLevel += (voice.bloomTarget - voice.bloomLevel) * voice.bloomCoeff;
                    voice.bloomLevel = flushDenormal (voice.bloomLevel);
                }
                else
                {
                    voice.bloomLevel = 1.0f;
                }

                // Hammer excitation
                float excitation = voice.hammer.process();

                // Modal resonator bank — the core of XOven
                float resonanceSum = 0.0f;
                for (int m = 0; m < kNumModes; ++m)
                {
                    // Dynamic pruning: skip modes that have decayed below threshold
                    if (voice.modes[m].amplitude < 1e-6f && !voice.hammer.active)
                        continue;

                    // Mode frequency from cast iron ratios
                    float modeFreq = freq * kCastIronModeRatios[m];

                    // Q: cast iron = very high Q (energy trapped)
                    // Base Q scales with density parameter: 150 (low density) to 600 (max)
                    float baseQ = 150.0f + densNow * 450.0f;
                    // Higher modes have slightly lower Q (radiation loss increases with freq)
                    float modeQ = baseQ / (1.0f + static_cast<float> (m) * 0.15f);
                    // Body resonance parameter boosts Q further
                    modeQ *= (1.0f + bodyRNow * 0.5f);
                    // LFO2 modulates body resonance
                    modeQ *= (1.0f + lfo2Val * 0.2f);

                    voice.modes[m].setFreqAndDecay (modeFreq, modeQ, srf);

                    // D001: velocity -> hammer hardness -> mode excitation spectrum
                    // Hard strikes excite all modes. Soft strikes only lower modes.
                    float modeAmp = kAudsleyAmplitudes[m];
                    float hardnessFalloff = 1.0f / (1.0f + static_cast<float> (m) * (2.0f - hardNow * 1.8f));
                    modeAmp *= hardnessFalloff;

                    // Cast iron character: upper modes decay faster than lower modes
                    // (material absorption is frequency-dependent)
                    float freqDecayFactor = fastExp (-0.0003f * static_cast<float> (m * m));
                    modeAmp *= freqDecayFactor;

                    resonanceSum += voice.modes[m].process (excitation) * modeAmp;
                }

                // Scale modal output
                resonanceSum *= 0.15f;

                // HF noise fill above modal ceiling
                float hfNoise = voice.hfFill.process (
                    std::min (brightNow + lfo1Val * 2000.0f, 14000.0f), srf) * pHFAmount;

                float voiceOut = (resonanceSum + hfNoise) * voice.bloomLevel;

                // Amplitude envelope
                float ampEnvLevel = voice.ampEnv.process();
                if (ampEnvLevel < 1e-7f && !voice.noteHeld && !voice.sustained)
                {
                    voice.active = false;
                    continue;
                }

                // Filter envelope: D001 velocity-scaled filter sweep
                float filterEnvMod = voice.filterEnv.process() * pFilterEnvAmt * 6000.0f * voice.velocity;
                float cutoff = std::clamp (
                    brightNow + filterEnvMod + lfo1Val * 3000.0f,
                    200.0f, 16000.0f);

                voice.outputFilter.setMode (CytomicSVF::Mode::LowPass);
                voice.outputFilter.setCoefficients (cutoff, 0.15f, srf);  // low resonance — piano filters are gentle
                float filtered = voice.outputFilter.processSample (voiceOut);

                float output = filtered * ampEnvLevel * voice.velocity;

                // Accumulate for sympathetic network (shared, post-mix)
                sympatheticDrive += output;

                // Stereo: spread voices across field using cached pan gains
                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            // Sympathetic string network (shared, post-mix, Decision K2)
            // couplingResonance boosts sympathetic depth when coupled to another Kitchen engine
            float effectiveSympDrive = sympatheticDrive * (1.0f + couplingResLevel * 0.5f);
            float sympatheticOut = sympatheticNetwork.process (effectiveSympDrive, sympNow);
            mixL += sympatheticOut * 0.5f;  // center-ish in stereo
            mixR += sympatheticOut * 0.5f;

            // Competition: adversarial soft saturation (cast iron under duress)
            if (competitionLevel > 0.01f)
            {
                mixL = fastTanh (mixL * (1.0f + competitionLevel * 3.0f)) / (1.0f + competitionLevel * 3.0f);
                mixR = fastTanh (mixR * (1.0f + competitionLevel * 3.0f)) / (1.0f + competitionLevel * 3.0f);
            }

            outL[s] = mixL;
            if (outR) outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        int count = 0;
        for (const auto& v : voices) if (v.active) ++count;
        activeVoiceCount.store (count);
        analyzeForSilenceGate (buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn (int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice (voices, kMaxVoices);
        auto& v = voices[idx];

        float freq = 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);

        float hardness = paramHardness ? paramHardness->load() : 0.4f;
        float brightness = paramBrightness ? paramBrightness->load() : 3000.0f;
        float bloomTime = paramBloomTime ? paramBloomTime->load() : 0.0f;
        float density = paramDensity ? paramDensity->load() : 0.7f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.noteHeld = true;
        v.sustained = false;
        v.glide.snapTo (freq);

        // D001: velocity controls hammer hardness
        float effectiveHardness = std::clamp (hardness + vel * 0.5f, 0.0f, 1.0f);
        v.hammer.trigger (vel, effectiveHardness, freq, srf);

        // HF noise fill
        v.hfFill.trigger (vel, brightness);
        v.hfFill.reset();  // reset SVF state for clean start

        // Filter envelope: fast attack, velocity-scaled decay
        v.filterEnv.prepare (srf);
        float filterDecay = 0.2f + (1.0f - vel) * 0.8f;  // hard strikes = fast decay (Maillard sear), soft = slow
        v.filterEnv.setADSR (0.001f, filterDecay, 0.0f, 0.8f);
        v.filterEnv.triggerHard();

        // Amp envelope
        v.ampEnv.prepare (srf);
        float ampAttack = paramAmpAttack ? paramAmpAttack->load() : 0.001f;
        float ampDecay = paramAmpDecay ? paramAmpDecay->load() : 2.0f;
        float ampSustain = paramAmpSustain ? paramAmpSustain->load() : 0.3f;
        float ampRelease = paramAmpRelease ? paramAmpRelease->load() : 1.5f;
        // Cast iron: density increases sustain (more energy trapped)
        ampDecay *= (0.5f + density * 1.5f);
        v.ampEnv.setADSR (ampAttack, ampDecay, ampSustain, ampRelease);
        v.ampEnv.triggerHard();

        // Bloom: slow attack ramp for cast iron thermal mass metaphor
        if (bloomTime > 0.01f)
        {
            v.bloomLevel = 0.0f;
            v.bloomTarget = 1.0f;
            v.bloomCoeff = 1.0f - std::exp (-1.0f / (bloomTime * 0.2f * srf));
        }
        else
        {
            v.bloomLevel = 1.0f;
            v.bloomTarget = 1.0f;
            v.bloomCoeff = 1.0f;
        }

        // Reset modal resonators
        for (auto& m : v.modes) m.reset();
        v.outputFilter.reset();

        // Stereo spread: distribute voices across the stereo field
        // Concert grand: lower notes left, higher notes right (player perspective)
        float normNote = (static_cast<float> (note) - 36.0f) / 60.0f;  // A1=36 to C8=96
        normNote = std::clamp (normNote, 0.0f, 1.0f);
        float panAngle = (normNote - 0.5f) * 0.6f;  // +-0.3 spread (subtle, realistic)
        v.panL = std::cos ((0.5f + panAngle * 0.5f) * 3.14159265f * 0.5f) * 1.414f;
        v.panR = std::sin ((0.5f + panAngle * 0.5f) * 3.14159265f * 0.5f) * 1.414f;

        // LFO phase stagger
        v.lfo1.reset (static_cast<float> (idx) / static_cast<float> (kMaxVoices));
        v.lfo2.reset (static_cast<float> (idx) / static_cast<float> (kMaxVoices) + 0.5f);

        // Update sympathetic network with new active notes
        updateSympatheticNotes();
    }

    void noteOff (int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.noteHeld = false;
                if (!damperPedal)
                {
                    v.ampEnv.release();
                    v.filterEnv.release();
                    v.sustained = false;
                }
                else
                {
                    // Damper pedal held: keep ringing (sustained)
                    v.sustained = true;
                }
            }
        }
        updateSympatheticNotes();
    }

    void handleDamperPedal (bool pressed) noexcept
    {
        damperPedal = pressed;
        if (!pressed)
        {
            // Pedal released: release all sustained notes
            for (auto& v : voices)
            {
                if (v.active && v.sustained && !v.noteHeld)
                {
                    v.ampEnv.release();
                    v.filterEnv.release();
                    v.sustained = false;
                }
            }
        }
        updateSympatheticNotes();
    }

    void updateSympatheticNotes() noexcept
    {
        int activeNotes[kMaxVoices];
        int numActive = 0;
        for (const auto& v : voices)
        {
            if (v.active)
                activeNotes[numActive++] = v.currentNote;
        }
        sympatheticNetwork.updateActiveStrings (activeNotes, numActive, srf);
    }

    //==========================================================================
    // Parameters — 27 total
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
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // === Modal Bank ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_brightness", 1 }, "Oven Brightness",
            juce::NormalisableRange<float> (200.0f, 16000.0f, 0.0f, 0.3f), 3000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_bodyResonance", 1 }, "Oven Body Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_hfAmount", 1 }, "Oven HF Noise Fill",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // === Hammer ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_hardness", 1 }, "Oven Hammer Hardness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        // === Material ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_density", 1 }, "Oven Density",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_temperature", 1 }, "Oven Temperature",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // === Sympathetic ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_sympathetic", 1 }, "Oven Sympathetic Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        // === Cast Iron Character ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_bloomTime", 1 }, "Oven Bloom Time",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_sustainTime", 1 }, "Oven Sustain Time",
            juce::NormalisableRange<float> (0.5f, 15.0f, 0.0f, 0.4f), 4.0f));

        // === Filter ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_filterEnvAmt", 1 }, "Oven Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        // === Amp ADSR ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_ampAttack", 1 }, "Oven Amp Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.3f), 0.001f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_ampDecay", 1 }, "Oven Amp Decay",
            juce::NormalisableRange<float> (0.05f, 10.0f, 0.0f, 0.4f), 2.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_ampSustain", 1 }, "Oven Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_ampRelease", 1 }, "Oven Amp Release",
            juce::NormalisableRange<float> (0.05f, 10.0f, 0.0f, 0.4f), 1.5f));

        // === Pitch ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_bendRange", 1 }, "Oven Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // === Macros ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_macroCharacter", 1 }, "Oven Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_macroMovement", 1 }, "Oven Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_macroCoupling", 1 }, "Oven Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_macroSpace", 1 }, "Oven Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // === LFOs ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_lfo1Rate", 1 }, "Oven LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_lfo1Depth", 1 }, "Oven LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.05f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "oven_lfo1Shape", 1 }, "Oven LFO1 Shape", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_lfo2Rate", 1 }, "Oven LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.8f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_lfo2Depth", 1 }, "Oven LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "oven_lfo2Shape", 1 }, "Oven LFO2 Shape", 0, 4, 0));

        // === Competition (adversarial coupling stub) ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_competition", 1 }, "Oven Competition",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oven_couplingResonance", 1 }, "Oven Coupling Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramBrightness      = apvts.getRawParameterValue ("oven_brightness");
        paramBodyResonance   = apvts.getRawParameterValue ("oven_bodyResonance");
        paramHFAmount        = apvts.getRawParameterValue ("oven_hfAmount");
        paramHardness        = apvts.getRawParameterValue ("oven_hardness");
        paramDensity         = apvts.getRawParameterValue ("oven_density");
        paramTemperature     = apvts.getRawParameterValue ("oven_temperature");
        paramSympathetic     = apvts.getRawParameterValue ("oven_sympathetic");
        paramBloomTime       = apvts.getRawParameterValue ("oven_bloomTime");
        paramSustainTime     = apvts.getRawParameterValue ("oven_sustainTime");
        paramFilterEnvAmt    = apvts.getRawParameterValue ("oven_filterEnvAmt");
        paramAmpAttack       = apvts.getRawParameterValue ("oven_ampAttack");
        paramAmpDecay        = apvts.getRawParameterValue ("oven_ampDecay");
        paramAmpSustain      = apvts.getRawParameterValue ("oven_ampSustain");
        paramAmpRelease      = apvts.getRawParameterValue ("oven_ampRelease");
        paramBendRange       = apvts.getRawParameterValue ("oven_bendRange");
        paramMacroCharacter  = apvts.getRawParameterValue ("oven_macroCharacter");
        paramMacroMovement   = apvts.getRawParameterValue ("oven_macroMovement");
        paramMacroCoupling   = apvts.getRawParameterValue ("oven_macroCoupling");
        paramMacroSpace      = apvts.getRawParameterValue ("oven_macroSpace");
        paramLfo1Rate        = apvts.getRawParameterValue ("oven_lfo1Rate");
        paramLfo1Depth       = apvts.getRawParameterValue ("oven_lfo1Depth");
        paramLfo1Shape       = apvts.getRawParameterValue ("oven_lfo1Shape");
        paramLfo2Rate        = apvts.getRawParameterValue ("oven_lfo2Rate");
        paramLfo2Depth       = apvts.getRawParameterValue ("oven_lfo2Depth");
        paramLfo2Shape       = apvts.getRawParameterValue ("oven_lfo2Shape");
        paramCompetition     = apvts.getRawParameterValue ("oven_competition");
        paramCouplingRes     = apvts.getRawParameterValue ("oven_couplingResonance");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OvenVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    OvenSympatheticNetwork sympatheticNetwork;

    ParameterSmoother smoothBrightness, smoothHardness, smoothDensity;
    ParameterSmoother smoothTemperature, smoothSympathetic, smoothBodyResonance;
    ParameterSmoother smoothBloom;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;
    bool damperPedal = false;

    // Thermal drift state — cast iron's massive thermal mass
    float thermalState = 0.0f, thermalTarget = 0.0f;
    float thermalCoeff = 0.0001f;
    int thermalTimer = 0;
    uint32_t thermalNoiseState = 54321u;

    // Coupling state
    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingBodyMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramBodyResonance = nullptr;
    std::atomic<float>* paramHFAmount = nullptr;
    std::atomic<float>* paramHardness = nullptr;
    std::atomic<float>* paramDensity = nullptr;
    std::atomic<float>* paramTemperature = nullptr;
    std::atomic<float>* paramSympathetic = nullptr;
    std::atomic<float>* paramBloomTime = nullptr;
    std::atomic<float>* paramSustainTime = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramAmpAttack = nullptr;
    std::atomic<float>* paramAmpDecay = nullptr;
    std::atomic<float>* paramAmpSustain = nullptr;
    std::atomic<float>* paramAmpRelease = nullptr;
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
    std::atomic<float>* paramCompetition = nullptr;
    std::atomic<float>* paramCouplingRes = nullptr;
};

} // namespace xolokun
