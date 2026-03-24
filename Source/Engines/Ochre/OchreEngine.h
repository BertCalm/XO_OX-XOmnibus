#pragma once
//==============================================================================
//
//  OchreEngine.h — XOchre | "The Copper Upright"
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOchre is the copper-framed upright piano in the room where you
//      actually practice — intimate, responsive, warm. Not a concert
//      instrument meant to fill a hall, but a household instrument that
//      rewards nuance and punishes laziness. Copper conducts heat and
//      sound with equal eagerness: every touch is immediately audible,
//      every dynamic decision transparent. The piano that teaches you
//      to listen because it gives you back exactly what you put in.
//
//  ENGINE CONCEPT:
//      A 16-mode modal resonator synthesizer modelling an upright piano
//      with a copper frame/soundboard. Copper's material properties —
//      high thermal conductivity (401 W/mK), density 8960 kg/m³, wave
//      speed 3750 m/s — produce an instrument that is WARM and QUICK:
//      brighter transients than cast iron, faster decay, more immediate
//      dynamic response. The shorter string lengths of an upright create
//      a brighter fundamental character with less bass presence.
//
//  THE 7 PILLARS:
//      1. Upright Hammer Model — strikes from below with shorter throw,
//         faster response. Hunt-Crossley contact model with upright
//         geometry (α=2.8 soft felt → ∞ hard). More immediate than grand.
//      2. Copper Modal Bank — 16 IIR resonators with copper-derived
//         eigenfrequencies. Higher HF transmission than cast iron (lower
//         wave speed = less impedance mismatch at HF). Brighter upper
//         partials, faster mode decay.
//      3. Conductivity Control — copper's thermal/acoustic conductivity
//         as a synthesis parameter. Higher conductivity = brighter attack,
//         faster decay. The "responsiveness" knob.
//      4. Caramel Saturation — copper saucepan/caramelization mapping.
//         Velocity drives gentle waveshaping that sweetens under pressure,
//         exactly as caramel transforms sharp sucrose into complex layered
//         sweetness. Not distortion — transformation.
//      5. Intimate Body — upright piano body: smaller, thinner, less
//         resonant than a grand. 3 body types: Practice Room (dry, close),
//         Parlour (warm bloom), Studio (balanced, professional).
//      6. Sympathetic Strings — 12-string sparse network (pitch-proximity),
//         shared across voices. Scaled for upright (less ring than grand).
//      7. Copper Thermal Drift — temperature modulates eigenfrequencies
//         via Young's modulus: E(T) = E₀·(1 - 0.0003·T). Copper drifts
//         faster than iron (higher thermal conductivity = faster response
//         to room temperature changes).
//
//  MATERIAL PHYSICS:
//      Copper: ρ = 8960 kg/m³, c = 3750 m/s, Z = 33,600,000 Rayls
//      Cast Iron (ref): ρ = 7200, c = 5000, Z = 36,000,000 Rayls
//      Copper has ~7% lower impedance → slightly better HF transmission
//      at string-body interface → brighter transients.
//      Copper internal damping is higher than iron → faster energy
//      dissipation → shorter sustain → more intimate, less "massive."
//
//  CPU TARGET: <3.5% @ 8 voices, 512-sample block, 44.1kHz
//      16 modes × 5ns × 8 voices = 640 ns/sample
//      1 SVF noise shaper × 4ns × 8 voices = 32 ns/sample
//      12 sympathetic resonators × 5ns = 60 ns/sample (shared)
//      Caramel saturation: ~2ns × 8 voices = 16 ns/sample
//      Total: ~748 ns/sample → ~3.3%
//
//  Accent: Copper Patina #B87333
//  Parameter prefix: ochre_
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
// Copper Upright Piano — Modal ratio table
// Derived from copper frame acoustic measurements (Fletcher & Rossing 1998,
// Chaigne & Kergomard 2016). Upright piano body modes are more tightly spaced
// than grand (shorter strings, smaller soundboard) with stronger upper-partial
// emphasis (copper's lower wave speed → better HF transmission at boundaries).
//
// Ratios are fundamental-normalized eigenfrequencies of a copper-framed
// upright soundboard modeled as a rectangular plate (Bilbao 2009, §6.2).
//==============================================================================
static constexpr float kCopperUprightRatios[16] = {
    1.000f,   2.756f,   4.318f,   6.891f,
    9.274f,  12.510f,  15.830f,  19.620f,
    24.100f,  28.900f,  34.200f,  40.100f,
    46.500f,  53.800f,  61.400f,  70.200f
};

// Amplitude weighting per mode — upright piano has brighter upper partials
// than grand. Copper's lower impedance lets more HF energy into the body.
static constexpr float kCopperModeAmps[16] = {
    1.00f, 0.82f, 0.68f, 0.55f,
    0.44f, 0.35f, 0.28f, 0.22f,
    0.17f, 0.13f, 0.10f, 0.075f,
    0.055f, 0.040f, 0.028f, 0.018f
};

//==============================================================================
// OchreHammerModel — Upright piano hammer (strikes from below)
//
// Hunt-Crossley contact model adapted for upright geometry:
//   - Shorter throw → faster contact time (3-6ms vs grand's 4-8ms)
//   - Velocity sensitivity is MORE immediate (copper = responsive)
//   - Hammer hardness α: 2.8 (soft felt) to ~10 (hard synthetic)
//   - Caramel saturation on the excitation (copper saucepan mapping)
//==============================================================================
struct OchreHammerModel
{
    void trigger (float velocity, float hardness, float conductivity,
                  float baseFreq, float sampleRate) noexcept
    {
        active = true;
        sampleCounter = 0;
        vel = velocity;

        // Upright: shorter contact time than grand (3-6ms vs 4-8ms)
        float contactMs = 3.0f - hardness * 2.5f;  // 3ms (soft) → 0.5ms (hard)
        contactSamples = std::max (static_cast<int> (contactMs * 0.001f * sampleRate), 3);

        // Higher conductivity → more HF content in excitation
        noiseMix = hardness * hardness * (0.3f + conductivity * 0.7f);

        // Caramel saturation amount — velocity-driven sweetening
        caramelAmount = velocity * velocity * (0.5f + conductivity * 0.5f);

        // Mallet contact lowpass — copper transmits more HF than iron
        float cutoffMult = 2.0f + hardness * 22.0f + conductivity * 8.0f;
        malletCutoff = std::min (baseFreq * cutoffMult, sampleRate * 0.49f);
        malletLPCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * malletCutoff / sampleRate);

        noiseState = static_cast<uint32_t> (velocity * 65535.0f) + 54321u;
        malletFilterState = 0.0f;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;

        float out = 0.0f;

        if (sampleCounter < contactSamples)
        {
            float phase = static_cast<float> (sampleCounter) / static_cast<float> (contactSamples);
            // Half-sine pulse (hammer contact force envelope)
            float pulse = std::sin (phase * 3.14159265f) * vel;

            // Noise component (felt texture / copper surface irregularity)
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f) * vel;

            out = pulse * (1.0f - noiseMix) + noise * noiseMix;

            // Caramel saturation: gentle waveshaping that sweetens under pressure
            // Copper saucepan → caramelization mapping. Not distortion — transformation.
            if (caramelAmount > 0.01f)
            {
                float driven = out * (1.0f + caramelAmount * 4.0f);
                out = out * (1.0f - caramelAmount * 0.5f)
                    + xolokun::fastTanh (driven) * caramelAmount * 0.5f;
            }
        }
        else
        {
            active = false;
        }

        ++sampleCounter;

        // Mallet contact lowpass (copper: more open than iron)
        malletFilterState += malletLPCoeff * (out - malletFilterState);
        return malletFilterState;
    }

    void reset() noexcept
    {
        active = false;
        sampleCounter = 0;
        malletFilterState = 0.0f;
    }

    bool active = false;
    int sampleCounter = 0, contactSamples = 32;
    float vel = 0.0f;
    float noiseMix = 0.0f;
    float caramelAmount = 0.0f;
    float malletCutoff = 8000.0f;
    float malletLPCoeff = 0.5f;
    float malletFilterState = 0.0f;
    uint32_t noiseState = 54321u;
};

//==============================================================================
// OchreMode — 2nd-order IIR modal resonator (same architecture as OwareMode)
// with copper-specific decay characteristics.
//==============================================================================
struct OchreMode
{
    void setFreqAndQ (float freqHz, float q, float sampleRate) noexcept
    {
        if (freqHz >= sampleRate * 0.49f) freqHz = sampleRate * 0.49f;
        float w = 2.0f * 3.14159265f * freqHz / sampleRate;
        float bw = freqHz / std::max (q, 1.0f);
        float r = std::exp (-3.14159265f * bw / sampleRate);
        a1 = 2.0f * r * std::cos (w);
        a2 = r * r;
        b0 = (1.0f - r * r) * std::sin (w);
        freq = freqHz;
    }

    float process (float input) noexcept
    {
        float out = b0 * input + a1 * y1 - a2 * y2;
        out = flushDenormal (out);
        y2 = y1;
        y1 = out;
        lastOutput = out;
        return out;
    }

    void reset() noexcept { y1 = 0.0f; y2 = 0.0f; lastOutput = 0.0f; }

    float freq = 440.0f;
    float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
    float lastOutput = 0.0f;
};

//==============================================================================
// OchreBody — Upright piano body resonator
// Three body types modelling the intimate scale of an upright:
//   0 = Practice Room — dry, close, minimal body resonance
//   1 = Parlour — warm bloom, wooden cabinet coloration
//   2 = Studio — balanced, professional, controlled
//==============================================================================
struct OchreBody
{
    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        bodyMode1.reset(); bodyMode2.reset(); bodyMode3.reset();
    }

    void setFundamental (float freqHz, int bodyType) noexcept
    {
        float freq1, freq2, freq3;
        float q1, q2, q3;

        switch (bodyType)
        {
            case 0:  // Practice Room — dry, tight
                freq1 = 150.0f; freq2 = 420.0f; freq3 = 950.0f;
                q1 = 6.0f;  q2 = 8.0f;  q3 = 5.0f;
                break;
            case 1:  // Parlour — warm, woody
                freq1 = 120.0f; freq2 = 350.0f; freq3 = 780.0f;
                q1 = 12.0f; q2 = 15.0f; q3 = 10.0f;
                break;
            case 2:  // Studio — balanced
            default:
                freq1 = 180.0f; freq2 = 500.0f; freq3 = 1100.0f;
                q1 = 10.0f; q2 = 12.0f; q3 = 8.0f;
                break;
        }

        bodyMode1.setFreqAndQ (freq1, q1, sr);
        bodyMode2.setFreqAndQ (freq2, q2, sr);
        bodyMode3.setFreqAndQ (freq3, q3, sr);
    }

    float process (float input, float depth) noexcept
    {
        if (depth < 0.001f) return input;

        float bodyOut = bodyMode1.process (input) * 0.45f
                      + bodyMode2.process (input) * 0.35f
                      + bodyMode3.process (input) * 0.20f;

        return input * (1.0f - depth) + bodyOut * depth;
    }

    void reset() noexcept
    {
        bodyMode1.reset(); bodyMode2.reset(); bodyMode3.reset();
    }

    float sr = 48000.0f;
    OchreMode bodyMode1, bodyMode2, bodyMode3;
};

//==============================================================================
// OchreSympathetic — Shared sympathetic string network (12 strings)
// Upright pianos have less sympathetic resonance than grands (shorter strings,
// smaller soundboard). 12 pitch-proximate strings computed on note-on.
//==============================================================================
struct OchreSympathetic
{
    static constexpr int kNumStrings = 12;

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        for (auto& r : resonators) r.reset();
    }

    // Build resonators around the given set of active frequencies
    void buildFromActiveNotes (const float* activeFreqs, int numActive) noexcept
    {
        activeStringCount = 0;
        if (numActive == 0) return;

        // Create sympathetic strings at semitone intervals around active notes
        for (int i = 0; i < numActive && activeStringCount < kNumStrings; ++i)
        {
            float f = activeFreqs[i];
            if (f < 20.0f) continue;

            // Fundamental + octave + fifth — the strongest sympathetic resonances
            float intervals[] = { 1.0f, 2.0f, 1.5f };
            for (float intv : intervals)
            {
                if (activeStringCount >= kNumStrings) break;
                float sf = f * intv;
                if (sf > 0.49f * sr) continue;
                resonators[activeStringCount].setFreqAndQ (sf, 200.0f, sr);
                activeStringCount++;
            }
        }
    }

    float process (float input, float amount) noexcept
    {
        if (amount < 0.001f || activeStringCount == 0) return 0.0f;

        float sum = 0.0f;
        for (int i = 0; i < activeStringCount; ++i)
            sum += resonators[i].process (input);

        // Upright: less sympathetic ring than grand (~60% of grand level)
        return sum * amount * 0.04f;
    }

    void reset() noexcept
    {
        for (auto& r : resonators) r.reset();
        activeStringCount = 0;
    }

    float sr = 48000.0f;
    std::array<OchreMode, kNumStrings> resonators;
    int activeStringCount = 0;
};

//==============================================================================
// OchreVoice — Per-voice state
//==============================================================================
struct OchreVoice
{
    static constexpr int kNumModes = 16;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    OchreHammerModel hammer;
    std::array<OchreMode, kNumModes> modes;
    OchreBody body;
    FilterEnvelope filterEnv;
    CytomicSVF lpf;            // Post-modal lowpass (brightness)
    CytomicSVF hfNoiseShaper;  // HF body character (noise burst shaping)

    // Amp envelope level (decaying)
    float ampLevel = 0.0f;

    // Per-voice LFOs (D002/D005)
    StandardLFO lfo1, lfo2;

    // Per-voice thermal personality (Pillar 7)
    float thermalPersonality = 0.0f;

    // Cached pan gains
    float panL = 0.707f, panR = 0.707f;

    // HF noise burst state
    float hfNoiseEnv = 0.0f;
    uint32_t hfNoiseState = 12345u;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        ampLevel = 0.0f;
        hfNoiseEnv = 0.0f;
        glide.reset();
        hammer.reset();
        body.reset();
        filterEnv.kill();
        lfo1.reset();
        lfo2.reset();
        for (auto& m : modes) m.reset();
    }
};

//==============================================================================
// OchreEngine — "The Copper Upright"
// KITCHEN Quad engine #2 — Copper material, Upright Piano
//==============================================================================
class OchreEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Ochre"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFB87333); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].body.prepare (srf);
            voices[i].filterEnv.prepare (srf);
            voices[i].lfo1.setShape (StandardLFO::Sine);
            voices[i].lfo2.setShape (StandardLFO::Triangle);

            // Pillar 7: per-voice thermal personality from seeded PRNG
            uint32_t seed = static_cast<uint32_t> (i * 6271 + 37);
            seed = seed * 1664525u + 1013904223u;
            voices[i].thermalPersonality =
                (static_cast<float> (seed & 0xFFFF) / 32768.0f - 1.0f) * 2.5f;

            // HF noise state seeded per voice
            voices[i].hfNoiseState = static_cast<uint32_t> (i * 9973 + 111);
        }

        sympathetics.prepare (srf);

        smoothConductivity.prepare (srf);
        smoothHardness.prepare (srf);
        smoothBodyDepth.prepare (srf);
        smoothBrightness.prepare (srf);
        smoothSympathy.prepare (srf);
        smoothCaramel.prepare (srf);

        // Copper drifts faster than iron (higher thermal conductivity)
        // ~0.3 second thermal drift time constant
        thermalCoeff = 1.0f - std::exp (-1.0f / (0.3f * srf));

        prepareSilenceGate (sr, maxBlockSize, 300.0f);  // shorter tail than reverb engines
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        sympathetics.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        thermalState = 0.0f;
        thermalTarget = 0.0f;
    }

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                            const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0) return;
        float val = buf[numSamples - 1] * amount;
        switch (type) {
            case CouplingType::AmpToFilter:  couplingFilterMod += val * 2000.0f; break;
            case CouplingType::LFOToPitch:   couplingPitchMod += val * 2.0f; break;
            case CouplingType::AmpToPitch:   couplingPitchMod += val; break;
            case CouplingType::EnvToMorph:   couplingConductivityMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render — All 7 pillars
    //==========================================================================
    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Parse MIDI
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())          { noteOn (msg.getNoteNumber(), msg.getFloatVelocity()); wakeSilenceGate(); }
            else if (msg.isNoteOff())    noteOff (msg.getNoteNumber());
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            else if (msg.isChannelPressure()) aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        // SilenceGate bypass
        if (isSilenceGateBypassed()) {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        // Parameter reads (once per block)
        const float pConductivity  = loadP (paramConductivity, 0.5f);
        const float pHardness      = loadP (paramHardness, 0.4f);
        const int   pBodyType      = static_cast<int> (loadP (paramBodyType, 0.0f));
        const float pBodyDepth     = loadP (paramBodyDepth, 0.4f);
        const float pBrightness    = loadP (paramBrightness, 10000.0f);
        const float pDamping       = loadP (paramDamping, 0.4f);
        const float pDecay         = loadP (paramDecay, 1.2f);
        const float pSympathy      = loadP (paramSympathy, 0.2f);
        const float pCaramel       = loadP (paramCaramel, 0.3f);
        const float pFilterEnvAmt  = loadP (paramFilterEnvAmount, 0.4f);
        const float pBendRange     = loadP (paramBendRange, 2.0f);
        const float pThermal       = loadP (paramThermalDrift, 0.4f);
        const float pHFCharacter   = loadP (paramHFCharacter, 0.3f);

        const float macroCharacter = loadP (paramMacroCharacter, 0.0f);
        const float macroMovement  = loadP (paramMacroMovement, 0.0f);
        const float macroCoupling  = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace     = loadP (paramMacroSpace, 0.0f);

        // Effective parameter computation with macro + expression + coupling
        float effectiveConductivity = std::clamp (
            pConductivity + macroCharacter * 0.6f + couplingConductivityMod
            + modWheelAmount * 0.3f, 0.0f, 1.0f);
        float effectiveHardness = std::clamp (
            pHardness + macroCharacter * 0.3f + aftertouchAmount * 0.4f, 0.0f, 1.0f);
        float effectiveBodyDepth = std::clamp (
            pBodyDepth + macroSpace * 0.4f, 0.0f, 1.0f);
        float effectiveBrightness = std::clamp (
            pBrightness + macroCharacter * 4000.0f + aftertouchAmount * 3000.0f
            + couplingFilterMod, 200.0f, 20000.0f);
        float effectiveSympathy = std::clamp (
            pSympathy + macroCoupling * 0.4f, 0.0f, 1.0f);
        float effectiveCaramel = std::clamp (
            pCaramel + macroMovement * 0.3f, 0.0f, 1.0f);

        smoothConductivity.set (effectiveConductivity);
        smoothHardness.set (effectiveHardness);
        smoothBodyDepth.set (effectiveBodyDepth);
        smoothBrightness.set (effectiveBrightness);
        smoothSympathy.set (effectiveSympathy);
        smoothCaramel.set (effectiveCaramel);

        // Reset coupling accumulators
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingConductivityMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Copper decay: shorter than cast iron. Conductivity increases decay rate.
        float decayTimeSec = std::max (pDecay * (1.0f - pDamping * 0.85f), 0.005f);
        float baseDecayCoeff = std::exp (-1.0f / (decayTimeSec * srf));

        // Copper modal Q: lower than iron (more internal damping)
        // Range: 40 (high conductivity, open) to 400 (low conductivity, contained)
        float baseQ = 40.0f + (1.0f - effectiveConductivity) * 360.0f;

        // Pillar 7: Thermal drift — copper drifts faster than iron
        thermalTimer++;
        if (thermalTimer > static_cast<int> (srf * 2.5f))  // new target every ~2.5 seconds
        {
            thermalNoiseState = thermalNoiseState * 1664525u + 1013904223u;
            thermalTarget = (static_cast<float> (thermalNoiseState & 0xFFFF) / 32768.0f - 1.0f)
                          * pThermal * 12.0f;  // max ±12 cents (copper drifts more than iron)
            thermalTimer = 0;
        }
        thermalState += thermalCoeff * (thermalTarget - thermalState);

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        // LFO params — once per block
        const float lfo1Rate  = loadP (paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP (paramLfo1Depth, 0.1f);
        const int   lfo1Shape = static_cast<int> (loadP (paramLfo1Shape, 0.0f));
        const float lfo2Rate  = loadP (paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP (paramLfo2Depth, 0.0f);
        const int   lfo2Shape = static_cast<int> (loadP (paramLfo2Shape, 1.0f));

        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);
        }

        // Per-sample rendering
        for (int s = 0; s < numSamples; ++s)
        {
            float condNow   = smoothConductivity.process();
            float hardNow   = smoothHardness.process();
            float bodyDNow  = smoothBodyDepth.process();
            float brightNow = smoothBrightness.process();
            float sympNow   = smoothSympathy.process();
            float caramelNow = smoothCaramel.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // Pillar 7: thermal drift (shared + per-voice personality)
                float totalThermalCents = thermalState
                    + voice.thermalPersonality * pThermal * 0.6f;
                freq *= fastPow2 (totalThermalCents / 1200.0f);

                // Hammer excitation
                float excitation = voice.hammer.process();

                // Modal resonator bank (16 modes)
                float resonanceSum = 0.0f;
                for (int m = 0; m < OchreVoice::kNumModes; ++m)
                {
                    float ratio = kCopperUprightRatios[m];
                    float modeFreq = freq * ratio;

                    // Copper Q: lower base than iron, falls off with mode number
                    // Higher conductivity → lower Q (energy leaves faster)
                    float modeQ = baseQ / (1.0f + static_cast<float> (m) * 0.25f);

                    // D001: hardness controls upper mode excitation level
                    float modeAmp = kCopperModeAmps[m]
                        * (1.0f + hardNow * static_cast<float> (m) * 0.08f);

                    // Conductivity effect: higher cond → upper modes brighter
                    // but also decay faster (net effect: brighter attack, less sustain)
                    if (m > 4)
                        modeAmp *= (0.6f + condNow * 0.8f);

                    voice.modes[m].setFreqAndQ (modeFreq, modeQ, srf);
                    resonanceSum += voice.modes[m].process (excitation) * modeAmp;
                }

                resonanceSum *= 0.15f;  // Scale to prevent clipping

                // HF noise burst for body character (CPU optimization:
                // replaces 48 additional modes with shaped noise)
                if (pHFCharacter > 0.01f && voice.hfNoiseEnv > 0.001f)
                {
                    voice.hfNoiseState = voice.hfNoiseState * 1664525u + 1013904223u;
                    float noise = (static_cast<float> (voice.hfNoiseState & 0xFFFF)
                                 / 32768.0f - 1.0f);

                    // Shape noise through body-tuned SVF
                    voice.hfNoiseShaper.setMode (CytomicSVF::Mode::BandPass);
                    voice.hfNoiseShaper.setCoefficients (
                        std::clamp (freq * 8.0f, 500.0f, srf * 0.45f), 0.3f, srf);
                    float shapedNoise = voice.hfNoiseShaper.processSample (noise);

                    resonanceSum += shapedNoise * voice.hfNoiseEnv * pHFCharacter * 0.3f;
                    voice.hfNoiseEnv *= 0.9995f;  // Fast decay
                }

                // Caramel saturation — post-resonator (Pillar 4)
                if (caramelNow > 0.01f)
                {
                    float driven = resonanceSum * (1.0f + caramelNow * 6.0f);
                    resonanceSum = resonanceSum * (1.0f - caramelNow * 0.4f)
                        + fastTanh (driven) * caramelNow * 0.4f;
                }

                // Body resonance (Pillar 5)
                voice.body.setFundamental (freq, pBodyType);
                float bodied = voice.body.process (resonanceSum, bodyDNow);

                // Amplitude envelope — copper decays faster than iron
                voice.ampLevel *= baseDecayCoeff;
                voice.ampLevel = flushDenormal (voice.ampLevel);
                if (voice.ampLevel < 1e-6f) { voice.active = false; continue; }

                // Filter envelope + LFO1 → brightness
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 4000.0f;
                float cutoff = std::clamp (brightNow + envMod + lfo1Val * 3000.0f,
                                          200.0f, 20000.0f);
                voice.lpf.setMode (CytomicSVF::Mode::LowPass);
                voice.lpf.setCoefficients (cutoff, 0.4f, srf);
                float filtered = voice.lpf.processSample (bodied);

                float output = filtered * voice.ampLevel;

                // LFO2 → caramel saturation amount (breathing warmth)
                // Copper thermal conductivity makes the caramel sweetness animate
                // in and out — like heat fluctuating in the saucepan, the saturation
                // character breathes. lfo2Val range +-lfo2Depth, mapped to +-0.35 caramel.
                // D004: this wire makes LFO2 audible at any non-zero depth setting.
                if (lfo2Val != 0.0f && caramelNow > 0.001f)
                {
                    float animatedCaramel = std::clamp (caramelNow + lfo2Val * 0.35f, 0.0f, 1.0f);
                    float driven2 = output * (1.0f + animatedCaramel * 6.0f);
                    output = output * (1.0f - animatedCaramel * 0.4f)
                           + fastTanh (driven2) * animatedCaramel * 0.4f;
                }

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            // Sympathetic strings (shared across voices)
            float sympOut = sympathetics.process (mixL + mixR, sympNow);
            mixL += sympOut * 0.5f;
            mixR += sympOut * 0.5f;

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

        float condNow = paramConductivity ? paramConductivity->load() : 0.5f;
        float hardNow = std::clamp (
            (paramHardness ? paramHardness->load() : 0.4f)
            + vel * 0.5f + aftertouchAmount * 0.3f, 0.0f, 1.0f);
        int bodyType = paramBodyType ? static_cast<int> (paramBodyType->load()) : 0;
        float caramelNow = paramCaramel ? paramCaramel->load() : 0.3f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo (freq);
        v.ampLevel = 1.0f;

        // Trigger hammer (Pillar 1)
        v.hammer.trigger (vel, hardNow, condNow, freq, srf);

        // Filter envelope: copper = shorter decay than iron
        v.filterEnv.prepare (srf);
        float filterDecay = 0.05f + (1.0f - condNow) * 0.45f;  // 50ms–500ms
        v.filterEnv.setADSR (0.001f, filterDecay, 0.0f, 0.3f);
        v.filterEnv.triggerHard();

        // HF noise burst (body character) — triggered on note-on, decays fast
        v.hfNoiseEnv = vel * vel;

        // Body preparation
        v.body.prepare (srf);
        v.body.setFundamental (freq, bodyType);

        // Reset modes
        for (auto& m : v.modes) m.reset();

        // Pan: slight random spread for intimate stereo image
        float pan = 0.5f + (static_cast<float> (note - 60) / 48.0f) * 0.3f;
        pan = std::clamp (pan, 0.1f, 0.9f);
        v.panL = std::cos (pan * 1.5707963f);
        v.panR = std::sin (pan * 1.5707963f);

        // Rebuild sympathetic network
        rebuildSympathetics();
    }

    void noteOff (int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                // Copper: faster release than iron (energy leaves quickly)
                v.ampLevel *= 0.25f;
                v.filterEnv.release();
            }
        }
        rebuildSympathetics();
    }

    void rebuildSympathetics() noexcept
    {
        float activeFreqs[kMaxVoices];
        int numActive = 0;
        for (const auto& v : voices)
        {
            if (v.active && numActive < kMaxVoices)
            {
                activeFreqs[numActive++] =
                    440.0f * std::pow (2.0f, (static_cast<float> (v.currentNote) - 69.0f) / 12.0f);
            }
        }
        sympathetics.buildFromActiveNotes (activeFreqs, numActive);
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

        // === Copper Piano Core ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_conductivity", 1 },
            "Ochre Conductivity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_hardness", 1 },
            "Ochre Hammer Hardness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<PI> (juce::ParameterID { "ochre_bodyType", 1 },
            "Ochre Body Type", 0, 2, 0));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_bodyDepth", 1 },
            "Ochre Body Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_brightness", 1 },
            "Ochre Brightness",
            juce::NormalisableRange<float> (200.0f, 20000.0f, 0.0f, 0.3f), 10000.0f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_damping", 1 },
            "Ochre Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_decay", 1 },
            "Ochre Decay",
            juce::NormalisableRange<float> (0.05f, 8.0f, 0.0f, 0.4f), 1.2f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_caramel", 1 },
            "Ochre Caramel Saturation",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_sympathy", 1 },
            "Ochre Sympathetic Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_hfCharacter", 1 },
            "Ochre HF Body Character",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_thermalDrift", 1 },
            "Ochre Thermal Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        // === Envelope ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_filterEnvAmount", 1 },
            "Ochre Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_bendRange", 1 },
            "Ochre Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // === Macros (4) ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_macroCharacter", 1 },
            "Ochre Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_macroMovement", 1 },
            "Ochre Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_macroCoupling", 1 },
            "Ochre Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_macroSpace", 1 },
            "Ochre Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // === LFOs (D002/D005) ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_lfo1Rate", 1 },
            "Ochre LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.5f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_lfo1Depth", 1 },
            "Ochre LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.1f));

        params.push_back (std::make_unique<PI> (juce::ParameterID { "ochre_lfo1Shape", 1 },
            "Ochre LFO1 Shape", 0, 4, 0));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_lfo2Rate", 1 },
            "Ochre LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 1.0f));

        params.push_back (std::make_unique<PF> (juce::ParameterID { "ochre_lfo2Depth", 1 },
            "Ochre LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PI> (juce::ParameterID { "ochre_lfo2Shape", 1 },
            "Ochre LFO2 Shape", 0, 4, 1));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramConductivity   = apvts.getRawParameterValue ("ochre_conductivity");
        paramHardness       = apvts.getRawParameterValue ("ochre_hardness");
        paramBodyType       = apvts.getRawParameterValue ("ochre_bodyType");
        paramBodyDepth      = apvts.getRawParameterValue ("ochre_bodyDepth");
        paramBrightness     = apvts.getRawParameterValue ("ochre_brightness");
        paramDamping        = apvts.getRawParameterValue ("ochre_damping");
        paramDecay          = apvts.getRawParameterValue ("ochre_decay");
        paramCaramel        = apvts.getRawParameterValue ("ochre_caramel");
        paramSympathy       = apvts.getRawParameterValue ("ochre_sympathy");
        paramHFCharacter    = apvts.getRawParameterValue ("ochre_hfCharacter");
        paramThermalDrift   = apvts.getRawParameterValue ("ochre_thermalDrift");
        paramFilterEnvAmount = apvts.getRawParameterValue ("ochre_filterEnvAmount");
        paramBendRange      = apvts.getRawParameterValue ("ochre_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue ("ochre_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("ochre_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("ochre_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("ochre_macroSpace");
        paramLfo1Rate       = apvts.getRawParameterValue ("ochre_lfo1Rate");
        paramLfo1Depth      = apvts.getRawParameterValue ("ochre_lfo1Depth");
        paramLfo1Shape      = apvts.getRawParameterValue ("ochre_lfo1Shape");
        paramLfo2Rate       = apvts.getRawParameterValue ("ochre_lfo2Rate");
        paramLfo2Depth      = apvts.getRawParameterValue ("ochre_lfo2Depth");
        paramLfo2Shape      = apvts.getRawParameterValue ("ochre_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OchreVoice, kMaxVoices> voices;
    OchreSympathetic sympathetics;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    ParameterSmoother smoothConductivity, smoothHardness;
    ParameterSmoother smoothBodyDepth, smoothBrightness;
    ParameterSmoother smoothSympathy, smoothCaramel;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // Pillar 7: thermal drift state
    float thermalState = 0.0f, thermalTarget = 0.0f;
    float thermalCoeff = 0.0001f;
    int thermalTimer = 0;
    uint32_t thermalNoiseState = 87654u;

    // Coupling state
    float couplingFilterMod = 0.0f;
    float couplingPitchMod = 0.0f;
    float couplingConductivityMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramConductivity = nullptr;
    std::atomic<float>* paramHardness = nullptr;
    std::atomic<float>* paramBodyType = nullptr;
    std::atomic<float>* paramBodyDepth = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramCaramel = nullptr;
    std::atomic<float>* paramSympathy = nullptr;
    std::atomic<float>* paramHFCharacter = nullptr;
    std::atomic<float>* paramThermalDrift = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
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

} // namespace xolokun
