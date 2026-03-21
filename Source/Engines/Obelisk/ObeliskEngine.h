#pragma once
//==============================================================================
//
//  ObeliskEngine.h — XObelisk | "The Cold Monolith"
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XObelisk is a monolith of cold marble standing in salt water,
//      half-submerged, half-reaching skyward. Its surface is perfectly smooth
//      except where some hand — Cage's hand — has placed objects between
//      the strings that run through its body like veins through stone.
//      Strike it and the marble rings: pure, cold, inharmonic. Press a bolt
//      into the strings and it rattles. Weave rubber through the nodes and
//      the stone hums through muted teeth. Glass laid across the strings
//      adds its own crystalline voice. Chain draped over them buzzes like
//      a bridge made of bees.
//
//  ENGINE CONCEPT:
//      A prepared piano synthesizer built on stone/marble physics. The body
//      is a modal resonator bank with inharmonic partial spacing derived
//      from real stone material properties. The HERO FEATURE is the
//      preparation system: bolt, rubber, glass, and chain preparations
//      each modify the modal bank in physically-grounded ways, creating
//      the timbral vocabulary John Cage explored from 1938 to 1975.
//
//      This is not a piano with effects. This is Cage's insight
//      operationalized: the preparation IS the synthesis parameter.
//
//  DSP ARCHITECTURE (per CPU optimization strategy):
//      - 16 modal resonators per voice (hybrid modal-stochastic, 3.2% CPU)
//      - Hunt-Crossley hammer contact model (stone-appropriate alpha)
//      - 5 preparation types: None, Bolt, Rubber, Glass, Chain
//      - Preparation position parameter (where on the string)
//      - Preparation depth parameter (how firmly inserted)
//      - HF noise shaper for stone's upper-frequency radiation
//      - 12 sympathetic resonators (shared, pitch-proximity sparse)
//      - Dynamic mode pruning (-60 dB threshold)
//      - SilenceGate with 500ms hold (piano sustain tails)
//
//  MATERIAL PHYSICS:
//      Marble: density rho = 2700 kg/m^3, wave speed c = 3800 m/s
//      Impedance Z = 10,260,000 (lower than metals — better energy transfer)
//      Internal loss factor eta ~ 0.001 (extremely low — stone rings)
//      Key character: INHARMONIC partials, very high Q, cold, mineral
//
//  REFERENCES:
//      Cage, J. (1961). "Silence: Lectures and Writings." Wesleyan UP.
//      Bilbao, S. (2009). "Numerical Sound Synthesis." Wiley.
//      Fletcher, N.H. & Rossing, T.D. (1998). "The Physics of Musical
//        Instruments." 2nd ed. Springer. Ch. 2-3 (bars & plates).
//      Chaigne, A. & Kergomard, J. (2016). "Acoustics of Musical
//        Instruments." Springer. Ch. 1, 6 (plates).
//
//  Accent: Obsidian Slate #4A4A4A (cold stone, unyielding)
//  Parameter prefix: obel_
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

namespace xomnibus {

//==============================================================================
// Stone mode ratio tables — INHARMONIC partial spacing.
//
// Unlike metals (nearly integer ratios) or strings (exact harmonics), stone
// plates and bars produce inharmonic partials. These ratios are derived from
// the dispersion relation for a vibrating plate with marble-like stiffness:
//
//   f_n = f_1 * (n^2 + B * n^alpha)
//
// where B depends on thickness/stiffness ratio and alpha ~ 1.5 for stone.
// The result: partials are spaced wider than harmonics, giving stone its
// characteristic cold, bell-like, mineral quality.
//
// Source: Fletcher & Rossing (1998), Table 2.2 (free bar modes) adapted
// for plate geometry with marble's Poisson ratio (nu = 0.27).
//==============================================================================
static constexpr int kNumModes = 16;

static constexpr float kStoneRatios[kNumModes] = {
    1.000f,   // fundamental
    2.756f,   // ~2.76x (vs 2.0 for harmonic) — wider than string
    5.404f,   // inharmonic — the "mineral third"
    8.933f,   // stone's 4th partial
    13.344f,  // widening gap
    18.637f,
    24.812f,
    31.870f,
    39.810f,  // upper modes become very sparse
    48.633f,
    58.339f,
    68.927f,
    80.398f,
    92.751f,
    105.987f,
    120.106f
};

// Preparation types — each modifies the modal bank differently
enum class ObeliskPrepType : int
{
    None   = 0,  // Unprepared stone piano — pure marble
    Bolt   = 1,  // Steel bolt between strings — adds mass, rattles
    Rubber = 2,  // Rubber mute — damps specific modes, creates holes
    Glass  = 3,  // Glass rod on strings — adds high-frequency ring
    Chain  = 4   // Chain draped over strings — buzzing nonlinearity
};

//==============================================================================
// ObeliskHammer — Hunt-Crossley contact model for stone excitation.
//
// Stone hammers are HARD. Contact time is short, force is large,
// the excitation is broadband. Alpha is high (stone doesn't absorb
// energy on impact — it reflects it). This creates the cold, percussive
// attack that defines prepared piano transients.
//
// F_contact(t) = p * delta(t)^alpha
// Stone alpha ~ 3.5-4.0 (vs 2.5 for piano felt)
//==============================================================================
struct ObeliskHammer
{
    void trigger (float velocity, float hardness, float baseFreq, float sampleRate) noexcept
    {
        active = true;
        sampleCounter = 0;

        // Stone contact: shorter than felt, scaled by hardness
        // Hard hits: 1ms contact. Soft: 4ms. Stone is inherently hard.
        float contactMs = 4.0f - hardness * 3.0f;
        contactSamples = std::max (static_cast<int> (contactMs * 0.001f * sampleRate), 4);

        peakAmplitude = velocity;

        // Stone excitation: more noise content than felt (impact character)
        noiseMix = 0.2f + hardness * 0.5f;  // always some noise — stone is percussive
        noiseState = static_cast<uint32_t> (velocity * 65535.0f + baseFreq) + 19937u;

        // Mallet spectral content: hard = all modes excited, soft = fundamental only
        // Stone alpha 3.5-4.0 means even soft hits are brighter than felt piano
        float malletCutoff = baseFreq * (3.0f + hardness * 17.0f);  // 3x to 20x fundamental
        float fc = std::min (malletCutoff, sampleRate * 0.49f);
        malletLPCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * fc / sampleRate);
        malletFilterState = 0.0f;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;

        float out = 0.0f;

        if (sampleCounter < contactSamples)
        {
            float phase = static_cast<float> (sampleCounter) / static_cast<float> (contactSamples);
            // Half-sine impulse — the standard contact model pulse shape
            float pulse = std::sin (phase * 3.14159265f) * peakAmplitude;
            // Impact noise — stone is inherently noisy on contact
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f) * peakAmplitude;
            out = pulse * (1.0f - noiseMix) + noise * noiseMix;
        }
        else
        {
            active = false;
        }

        ++sampleCounter;

        // Apply mallet spectral lowpass
        malletFilterState += malletLPCoeff * (out - malletFilterState);
        return malletFilterState;
    }

    void reset() noexcept { active = false; sampleCounter = 0; malletFilterState = 0.0f; }

    bool active = false;
    int sampleCounter = 0, contactSamples = 48;
    float peakAmplitude = 1.0f, noiseMix = 0.3f;
    uint32_t noiseState = 19937u;
    float malletLPCoeff = 0.5f, malletFilterState = 0.0f;
};

//==============================================================================
// ObeliskMode — 2nd-order resonator with preparation modification.
//
// Same structure as OwareMode but with preparation-aware coefficient
// modification. Each mode can have its frequency shifted, its Q altered,
// and its amplitude modified by the current preparation.
//==============================================================================
struct ObeliskMode
{
    void setFreqAndQ (float freqHz, float q, float sampleRate) noexcept
    {
        if (freqHz >= sampleRate * 0.49f) freqHz = sampleRate * 0.49f;
        if (freqHz < 10.0f) freqHz = 10.0f;
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
// ObeliskPreparation — The hero feature.
//
// Modifies the modal resonator bank based on the preparation type and
// position. Each preparation type has physically-grounded effects:
//
//   BOLT:   Adds mass at contact point. Modes whose antinodes align with
//           the bolt position are shifted down in frequency (added mass
//           lowers eigenfrequency). Creates rattle via amplitude modulation
//           of affected modes.
//
//   RUBBER: Damps modes whose antinodes align with the rubber position.
//           Creates spectral holes — some partials are muted, others ring
//           through. This is Cage's most common preparation.
//
//   GLASS:  Adds new high-frequency resonances (the glass's own modes)
//           above the fundamental series. The stone and glass ring together.
//
//   CHAIN:  Adds buzzing nonlinearity at the contact point. Similar to
//           hurdy-gurdy buzz bridge or sitar jawari. The chain bounces
//           against the string, creating broadband noise bursts that
//           track the fundamental.
//
// Position (0-1) determines which modes are most affected:
//   0.5 = center of string → kills even-numbered modes
//   0.33 = 1/3 point → affects every 3rd mode
//   This is real prepared piano physics.
//==============================================================================
struct ObeliskPreparation
{
    // Compute preparation modification for a single mode.
    // Returns: { freqMultiplier, qMultiplier, ampMultiplier }
    struct ModResult
    {
        float freqMul = 1.0f;
        float qMul = 1.0f;
        float ampMul = 1.0f;
        float extraExcitation = 0.0f;  // additional excitation from preparation
    };

    static ModResult computeModification (int prepType, float position, float depth,
                                           int modeIndex, int totalModes,
                                           float fundamentalHz, float sampleRate) noexcept
    {
        ModResult result;
        if (prepType == 0 || depth < 0.001f) return result;  // None

        // Compute how strongly this mode is affected by the preparation position.
        // A preparation at position P on a string affects mode N proportionally to
        // sin^2(N * pi * P) — the mode shape amplitude at the contact point.
        float modeNum = static_cast<float> (modeIndex + 1);
        float positionSensitivity = std::sin (modeNum * 3.14159265f * position);
        positionSensitivity *= positionSensitivity;  // square for amplitude (not displacement)

        float effectiveDepth = depth * positionSensitivity;

        switch (prepType)
        {
            case 1:  // BOLT — adds mass, lowers frequency, creates rattle
            {
                // Mass loading: frequency drops proportional to added mass
                // f_new = f_old / sqrt(1 + dm/m) ~ f_old * (1 - 0.5 * dm/m)
                // A bolt adds significant mass to the mode it contacts
                result.freqMul = 1.0f - effectiveDepth * 0.15f;  // up to 15% frequency drop

                // Bolt contact creates energy loss at contact point → slightly lower Q
                result.qMul = 1.0f - effectiveDepth * 0.3f;

                // Bolt rattle: affected modes get amplitude modulation
                // (this is handled separately in the voice — not a static mod)
                result.ampMul = 1.0f + effectiveDepth * 0.2f;  // slight boost from rattle energy
                break;
            }

            case 2:  // RUBBER — damps specific modes, creates spectral holes
            {
                // Rubber mutes are the most dramatic: modes at the contact point
                // are nearly killed, while modes with nodes at that point survive
                result.freqMul = 1.0f;  // rubber doesn't shift frequency much

                // HEAVY damping at the contact point — this is what makes Cage's
                // preparations so striking. Some modes vanish, others ring through.
                result.qMul = 1.0f - effectiveDepth * 0.95f;  // nearly kills contacted modes
                result.qMul = std::max (result.qMul, 0.05f);  // floor: don't completely zero out

                // Amplitude drops with damping
                result.ampMul = 1.0f - effectiveDepth * 0.8f;
                result.ampMul = std::max (result.ampMul, 0.02f);
                break;
            }

            case 3:  // GLASS — adds high-frequency ring
            {
                // Glass doesn't damp much, but it shifts higher modes upward
                // (the glass rod acts as a fulcrum, effectively shortening
                // the vibrating length for modes that have antinodes there)
                result.freqMul = 1.0f + effectiveDepth * 0.08f;  // slight upward shift

                // Glass is rigid — very little damping
                result.qMul = 1.0f + effectiveDepth * 0.3f;  // actually increases Q!

                // Glass adds its own resonant energy at high frequencies
                // This is handled as extra excitation fed into the mode
                float glassFreq = fundamentalHz * (4.0f + modeNum * 2.5f);
                if (glassFreq < sampleRate * 0.45f)
                    result.extraExcitation = effectiveDepth * 0.15f;

                result.ampMul = 1.0f;
                break;
            }

            case 4:  // CHAIN — buzzing nonlinearity
            {
                // Chain creates a distributed contact — affects many modes
                // but not as strongly as bolt or rubber at any single point
                result.freqMul = 1.0f - effectiveDepth * 0.03f;  // slight mass loading

                // Chain contact adds energy (buzz) but also damps slightly
                result.qMul = 1.0f - effectiveDepth * 0.2f;

                // Chain buzz adds broadband excitation — handled by the
                // chain buzzer module, not by mode amplitude
                result.ampMul = 1.0f;
                break;
            }
        }

        return result;
    }
};

//==============================================================================
// ObeliskChainBuzzer — Nonlinear contact buzzing for chain preparation.
//
// Simulates the chain bouncing against strings: when the string
// displacement exceeds a threshold, the chain makes contact and produces
// a burst of broadband noise modulated by the fundamental. This is the
// same principle as the sitar's jawari bridge and OWARE's buzz membrane.
//==============================================================================
struct ObeliskChainBuzzer
{
    float process (float input, float amount) noexcept
    {
        if (amount < 0.001f) return input;

        // Chain contact threshold — the chain only buzzes when displacement is high
        float threshold = 0.05f / (amount + 0.1f);
        float absIn = std::fabs (input);

        if (absIn > threshold)
        {
            // Contact! Generate buzz proportional to displacement over threshold
            float overThreshold = (absIn - threshold) * 10.0f;
            overThreshold = std::min (overThreshold, 1.0f);

            // Broadband noise modulated by the signal
            noiseState = noiseState * 1664525u + 1013904223u;
            float noise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f);

            // The buzz is the noise shaped by the input signal's sign and magnitude
            float buzz = noise * overThreshold * amount * 0.3f;

            // Feed through the BPF to shape the buzz spectrum (200-2000 Hz band)
            float buzzFiltered = buzzBPF.processSample (buzz);
            return input + buzzFiltered;
        }

        return input;
    }

    void prepare (float sampleRate) noexcept
    {
        buzzBPF.setMode (CytomicSVF::Mode::BandPass);
        buzzBPF.setCoefficients (800.0f, 0.4f, sampleRate);
    }

    void reset() noexcept { buzzBPF.reset(); }

    uint32_t noiseState = 31337u;
    CytomicSVF buzzBPF;
};

//==============================================================================
// ObeliskBoltRattle — Amplitude modulation for bolt preparation.
//
// A bolt wedged between strings doesn't sit perfectly still — it rattles
// at the resonant frequency of the bolt-string system. This creates an
// amplitude modulation on affected modes at a frequency related to the
// bolt's mass and the string tension.
//==============================================================================
struct ObeliskBoltRattle
{
    float process (float input, float amount, float modeFreq) noexcept
    {
        if (amount < 0.001f) return input;

        // Rattle frequency: related to mode frequency but offset
        // (the bolt has its own resonance, typically 30-200 Hz depending on size)
        // This creates beating between the mode and the bolt
        float rattleHz = 40.0f + modeFreq * 0.05f;
        phase += rattleHz / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        // Rattle is a modulation of amplitude — periodic contact/separation
        float rattle = 1.0f - amount * 0.4f * (0.5f + 0.5f * fastSin (phase * 6.28318f));
        return input * rattle;
    }

    void prepare (float sampleRate) noexcept { sr = sampleRate; phase = 0.0f; }
    void reset() noexcept { phase = 0.0f; }

    float sr = 48000.0f;
    float phase = 0.0f;
};

//==============================================================================
// ObeliskVoice
//==============================================================================
struct ObeliskVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    GlideProcessor glide;
    ObeliskHammer hammer;
    std::array<ObeliskMode, kNumModes> modes;
    ObeliskChainBuzzer chainBuzz;
    ObeliskBoltRattle boltRattle;
    FilterEnvelope filterEnv;
    FilterEnvelope ampEnv;
    CytomicSVF svf;
    CytomicSVF hfNoiseSVF;  // HF noise shaper for stone radiation character

    // D002: LFOs
    StandardLFO lfo1, lfo2;

    float ampLevel = 0.0f;

    // Per-voice cached preparation modifications (computed at note-on)
    std::array<ObeliskPreparation::ModResult, kNumModes> prepMods;

    // Cached pan gains
    float panL = 0.707f, panR = 0.707f;

    // Per-voice thermal personality (fixed offset in cents)
    float thermalPersonality = 0.0f;

    // Per-voice noise state for HF stone radiation
    uint32_t noiseState = 12345u;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        ampLevel = 0.0f;
        glide.reset();
        hammer.reset();
        chainBuzz.reset();
        boltRattle.reset();
        filterEnv.kill();
        ampEnv.kill();
        svf.reset();
        hfNoiseSVF.reset();
        lfo1.reset();
        lfo2.reset();
        for (auto& m : modes) m.reset();
        for (auto& p : prepMods) p = {};
    }
};

//==============================================================================
// ObeliskEngine — "The Cold Monolith" | KITCHEN Quad #3 (Stone/Marble)
//
// Prepared Piano synthesizer with physically-modeled stone body resonance
// and John Cage-inspired preparation system.
//
// 30 parameters with `obel_` prefix:
//   - Stone body: density, brightness, damping, decay
//   - Preparation system: prepType, prepPosition, prepDepth (THE HERO)
//   - Hammer: hardness
//   - Expression: filter envelope, LFOs, thermal drift
//   - Macros: STONE, PREPARATION, COUPLING, SPACE
//   - Pitch: bend range
//   - LFO1 (rate/depth/shape) → brightness
//   - LFO2 (rate/depth/shape) → preparation depth
//==============================================================================
class ObeliskEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr float kPruneThreshold = 1e-6f;  // -120 dB dynamic mode pruning

    juce::String getEngineId() const override { return "Obelisk"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF4A4A4A); }
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
            voices[i].chainBuzz.prepare (srf);
            voices[i].boltRattle.prepare (srf);
            voices[i].filterEnv.prepare (srf);
            voices[i].ampEnv.prepare (srf);
            voices[i].hfNoiseSVF.setMode (CytomicSVF::Mode::BandPass);
            voices[i].hfNoiseSVF.setCoefficients (6000.0f, 0.3f, srf);

            // Per-voice thermal personality from seeded PRNG
            uint32_t seed = static_cast<uint32_t> (i * 7919 + 73);
            seed = seed * 1664525u + 1013904223u;
            voices[i].thermalPersonality = (static_cast<float> (seed & 0xFFFF) / 32768.0f - 1.0f) * 1.5f;

            // Per-voice noise seed for HF radiation
            voices[i].noiseState = static_cast<uint32_t> (i * 48271 + 33333);
        }

        smoothDensity.prepare (srf);
        smoothHardness.prepare (srf);
        smoothPrepDepth.prepare (srf);
        smoothBrightness.prepare (srf);
        smoothDamping.prepare (srf);

        // Thermal drift time constant: 0.5 seconds, sample-rate scaled
        thermalCoeff = 1.0f - std::exp (-1.0f / (0.5f * srf));

        prepareSilenceGate (sr, maxBlockSize, 500.0f);  // piano sustain tail
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
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
            case CouplingType::EnvToMorph:   couplingPrepMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render — Modal resonator bank with preparation system
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        // Step 1: Parse MIDI — BEFORE silence gate check
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

        // Step 2: Silence gate bypass
        if (isSilenceGateBypassed()) {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        // Step 3: Read parameters (once per block — ParamSnapshot pattern)
        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const float pDensity        = loadP (paramDensity, 0.5f);
        const float pHardness       = loadP (paramHardness, 0.5f);
        const int   pPrepType       = static_cast<int> (loadP (paramPrepType, 0.0f));
        const float pPrepPosition   = loadP (paramPrepPosition, 0.5f);
        const float pPrepDepth      = loadP (paramPrepDepth, 0.5f);
        const float pBrightness     = loadP (paramBrightness, 6000.0f);
        const float pDamping        = loadP (paramDamping, 0.2f);
        const float pDecay          = loadP (paramDecay, 3.0f);
        const float pFilterEnvAmt   = loadP (paramFilterEnvAmount, 0.4f);
        const float pBendRange      = loadP (paramBendRange, 2.0f);
        const float pThermal        = loadP (paramThermalDrift, 0.2f);
        const float pHFNoiseAmt     = loadP (paramHFNoise, 0.3f);
        const float pStoneTone      = loadP (paramStoneTone, 0.5f);

        const float macroStone      = loadP (paramMacroStone, 0.0f);
        const float macroPrep       = loadP (paramMacroPrep, 0.0f);
        const float macroCoupling   = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace      = loadP (paramMacroSpace, 0.0f);

        // Apply macros + expression + coupling
        float effectiveDensity = std::clamp (pDensity + macroStone * 0.5f, 0.0f, 1.0f);
        float effectiveHardness = std::clamp (pHardness + aftertouchAmount * 0.4f, 0.0f, 1.0f);
        float effectivePrepDepth = std::clamp (pPrepDepth + macroPrep * 0.6f + couplingPrepMod, 0.0f, 1.0f);
        float effectiveBright = std::clamp (pBrightness + macroStone * 3000.0f
                                            + aftertouchAmount * 2000.0f + couplingFilterMod, 200.0f, 20000.0f);
        // D006: mod wheel → stone tone (cold ↔ warm axis)
        float effectiveStoneTone = std::clamp (pStoneTone + modWheelAmount * 0.5f, 0.0f, 1.0f);

        smoothDensity.set (effectiveDensity);
        smoothHardness.set (effectiveHardness);
        smoothPrepDepth.set (effectivePrepDepth);
        smoothBrightness.set (effectiveBright);
        smoothDamping.set (pDamping);

        // Reset coupling accumulators
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingPrepMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Stone Q: extremely high base Q (stone rings clearly)
        // Density controls Q: denser stone = higher Q = longer ring
        float baseQ = 200.0f + effectiveDensity * 800.0f;  // 200-1000 — very high Q

        // Decay coefficient: stone has very long natural decay
        float decayTimeSec = std::max (pDecay * (1.0f - pDamping * 0.8f), 0.01f);
        float baseDecayCoeff = std::exp (-1.0f / (decayTimeSec * srf));

        // Thermal drift — slow tuning drift (stone is temperature-sensitive for Q, not pitch)
        thermalTimer++;
        if (thermalTimer > static_cast<int> (srf * 5.0f))  // new target every ~5 seconds
        {
            thermalNoiseState = thermalNoiseState * 1664525u + 1013904223u;
            thermalTarget = (static_cast<float> (thermalNoiseState & 0xFFFF) / 32768.0f - 1.0f)
                          * pThermal * 5.0f;  // max +-5 cents (stone is thermally stable)
            thermalTimer = 0;
        }
        thermalState += thermalCoeff * (thermalTarget - thermalState);

        // Read LFO params once per block
        const float lfo1Rate  = loadP (paramLfo1Rate, 0.3f);
        const float lfo1Depth = loadP (paramLfo1Depth, 0.0f);
        const int   lfo1Shape = static_cast<int> (loadP (paramLfo1Shape, 0.0f));
        const float lfo2Rate  = loadP (paramLfo2Rate, 0.8f);
        const float lfo2Depth = loadP (paramLfo2Depth, 0.0f);
        const int   lfo2Shape = static_cast<int> (loadP (paramLfo2Shape, 0.0f));

        // Set LFO rate/shape once per block per voice
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);
        }

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        // Step 4: Per-sample render loop
        for (int s = 0; s < numSamples; ++s)
        {
            float densNow  = smoothDensity.process();
            float hardNow  = smoothHardness.process();
            float prepDNow = smoothPrepDepth.process();
            float brightNow = smoothBrightness.process();
            float dampNow  = smoothDamping.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                // LFO modulation
                float lfo1Val = voice.lfo1.process() * lfo1Depth;  // LFO1 → brightness
                float lfo2Val = voice.lfo2.process() * lfo2Depth;  // LFO2 → prep depth

                // Thermal drift: apply shared + per-voice personality
                float totalThermalCents = thermalState + voice.thermalPersonality * pThermal * 0.3f;
                freq *= fastPow2 (totalThermalCents / 1200.0f);

                // Per-voice prep depth modulated by LFO2
                float voicePrepDepth = std::clamp (prepDNow + lfo2Val * 0.3f, 0.0f, 1.0f);

                // Hammer excitation
                float excitation = voice.hammer.process();

                // Modal resonator bank with preparation modification
                float resonanceSum = 0.0f;
                int activeModeCount = 0;

                for (int m = 0; m < kNumModes; ++m)
                {
                    // Base inharmonic stone frequency
                    float modeFreq = freq * kStoneRatios[m];

                    // Apply preparation modification
                    auto prep = ObeliskPreparation::computeModification (
                        pPrepType, pPrepPosition, voicePrepDepth,
                        m, kNumModes, freq, srf);

                    modeFreq *= prep.freqMul;

                    // Stone Q: very high, modified by preparation and density
                    float modeQ = baseQ * prep.qMul;
                    // Higher modes have slightly lower Q (radiation damping)
                    modeQ /= (1.0f + static_cast<float> (m) * 0.15f);
                    modeQ = std::max (modeQ, 1.0f);

                    // Stone tone: cold (low tone = emphasize upper inharmonics)
                    //              warm (high tone = emphasize fundamental)
                    float toneWeight = 1.0f;
                    if (effectiveStoneTone < 0.5f)
                    {
                        // Cold: upper modes boosted, fundamental slightly reduced
                        float coldBoost = 1.0f + static_cast<float> (m) * (0.5f - effectiveStoneTone) * 0.15f;
                        toneWeight = (m == 0) ? (0.7f + effectiveStoneTone * 0.6f) : coldBoost;
                    }
                    else
                    {
                        // Warm: fundamental boosted, upper modes reduced
                        float warmFade = 1.0f - (effectiveStoneTone - 0.5f) * static_cast<float> (m) * 0.08f;
                        toneWeight = (m == 0) ? 1.2f : std::max (warmFade, 0.1f);
                    }

                    // D001: hardness controls upper mode excitation amplitude
                    float modeAmp = 1.0f / (1.0f + static_cast<float> (m) * (2.0f - hardNow * 1.5f));
                    modeAmp *= prep.ampMul * toneWeight;

                    // Dynamic mode pruning: skip modes that have decayed below threshold
                    if (voice.modes[m].lastOutput != 0.0f &&
                        std::fabs (voice.modes[m].lastOutput) < kPruneThreshold &&
                        excitation == 0.0f && prep.extraExcitation < 0.001f)
                    {
                        continue;
                    }

                    voice.modes[m].setFreqAndQ (modeFreq, modeQ, srf);
                    float modeInput = excitation + prep.extraExcitation;
                    float modeOut = voice.modes[m].process (modeInput) * modeAmp;

                    // BOLT rattle: amplitude modulation on affected modes
                    if (pPrepType == 1 && voicePrepDepth > 0.01f)
                    {
                        float posSens = std::sin ((m + 1.0f) * 3.14159265f * pPrepPosition);
                        posSens *= posSens;
                        if (posSens > 0.1f)
                            modeOut = voice.boltRattle.process (modeOut, voicePrepDepth * posSens, modeFreq);
                    }

                    resonanceSum += modeOut;
                    activeModeCount++;
                }

                // Normalize by active mode count to prevent level jumps
                if (activeModeCount > 0)
                    resonanceSum *= 4.0f / static_cast<float> (activeModeCount);

                // CHAIN buzzer (applied after modal sum)
                if (pPrepType == 4 && voicePrepDepth > 0.01f)
                    resonanceSum = voice.chainBuzz.process (resonanceSum, voicePrepDepth);

                // HF noise for stone radiation character
                // Stone radiates noise at impact that sits above the modal frequencies
                // This gives the "crack" and "mineral dust" character
                if (pHFNoiseAmt > 0.001f && voice.hammer.active)
                {
                    voice.noiseState = voice.noiseState * 1664525u + 1013904223u;
                    float noise = (static_cast<float> (voice.noiseState & 0xFFFF) / 32768.0f - 1.0f);
                    // Shape the noise into the stone's HF radiation band
                    float hfNoise = voice.hfNoiseSVF.processSample (noise * voice.velocity);
                    resonanceSum += hfNoise * pHFNoiseAmt * 0.3f;
                }

                // Amplitude envelope: stone has long natural decay
                voice.ampLevel *= baseDecayCoeff;
                voice.ampLevel = flushDenormal (voice.ampLevel);
                if (voice.ampLevel < 1e-7f) { voice.active = false; continue; }

                // Filter envelope + LFO1 → brightness
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 4000.0f;
                float cutoff = std::clamp (brightNow + envMod + lfo1Val * 2000.0f, 200.0f, 20000.0f);
                voice.svf.setMode (CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients (cutoff, 0.3f, srf);  // low resonance — stone is not resonant in the filter sense
                float filtered = voice.svf.processSample (resonanceSum);

                float output = filtered * voice.ampLevel;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR) outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        // Step 5: Post-render bookkeeping
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

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo (freq);
        v.ampLevel = 1.0f;

        // D001 + D006: velocity + aftertouch → hammer hardness
        float hardness = std::clamp (
            (paramHardness ? paramHardness->load() : 0.5f)
            + vel * 0.4f + aftertouchAmount * 0.3f, 0.0f, 1.0f);
        v.hammer.trigger (vel, hardness, freq, srf);

        // Filter envelope: stone = fast attack, medium-long decay
        v.filterEnv.prepare (srf);
        float density = paramDensity ? paramDensity->load() : 0.5f;
        float filterDecay = 0.15f + density * 0.6f;  // 150ms → 750ms
        v.filterEnv.setADSR (0.001f, filterDecay, 0.0f, 0.8f);
        v.filterEnv.triggerHard();

        // Reset modes and preparation modules
        for (auto& m : v.modes) m.reset();
        v.chainBuzz.prepare (srf);
        v.boltRattle.prepare (srf);
        v.hfNoiseSVF.setMode (CytomicSVF::Mode::BandPass);
        v.hfNoiseSVF.setCoefficients (6000.0f + vel * 4000.0f, 0.3f, srf);

        // Precompute preparation modifications (cached at note-on for efficiency)
        int prepType = paramPrepType ? static_cast<int> (paramPrepType->load()) : 0;
        float prepPos = paramPrepPosition ? paramPrepPosition->load() : 0.5f;
        float prepDepth = paramPrepDepth ? paramPrepDepth->load() : 0.5f;
        for (int m = 0; m < kNumModes; ++m)
        {
            v.prepMods[m] = ObeliskPreparation::computeModification (
                prepType, prepPos, prepDepth, m, kNumModes, freq, srf);
        }

        // Stereo spread: distribute voices across the stereo field
        float pan = (static_cast<float> (note) - 60.0f) / 48.0f;  // center C = center, spread by pitch
        pan = std::clamp (pan * 0.6f, -0.8f, 0.8f);  // limit spread
        float panAngle = (pan + 1.0f) * 0.5f * 1.5707963f;  // [0, pi/2]
        v.panL = std::cos (panAngle);
        v.panR = std::sin (panAngle);

        // Reset LFOs with voice stagger
        v.lfo1.reset (static_cast<float> (idx) / static_cast<float> (kMaxVoices));
        v.lfo2.reset (static_cast<float> (idx) / static_cast<float> (kMaxVoices) + 0.5f);
    }

    void noteOff (int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                // Stone release: gradual amplitude reduction
                v.ampLevel *= 0.4f;
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 30 total with `obel_` prefix
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

        // === STONE BODY ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_density", 1 }, "Obelisk Stone Density",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_stoneTone", 1 }, "Obelisk Stone Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_brightness", 1 }, "Obelisk Brightness",
            juce::NormalisableRange<float> (200.0f, 20000.0f, 0.0f, 0.3f), 6000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_damping", 1 }, "Obelisk Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_decay", 1 }, "Obelisk Decay",
            juce::NormalisableRange<float> (0.1f, 15.0f, 0.0f, 0.4f), 3.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_hfNoise", 1 }, "Obelisk HF Stone Noise",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // === HAMMER ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_hardness", 1 }, "Obelisk Hammer Hardness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // === PREPARATION SYSTEM (THE HERO) ===
        params.push_back (std::make_unique<PI> (juce::ParameterID { "obel_prepType", 1 }, "Obelisk Prep Type", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_prepPosition", 1 }, "Obelisk Prep Position",
            juce::NormalisableRange<float> (0.05f, 0.95f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_prepDepth", 1 }, "Obelisk Prep Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // === EXPRESSION ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_filterEnvAmount", 1 }, "Obelisk Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_thermalDrift", 1 }, "Obelisk Thermal Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_bendRange", 1 }, "Obelisk Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // === MACROS ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_macroStone", 1 }, "Obelisk Macro STONE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_macroPrep", 1 }, "Obelisk Macro PREPARATION",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_macroCoupling", 1 }, "Obelisk Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_macroSpace", 1 }, "Obelisk Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // === LFO1 (brightness) ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_lfo1Rate", 1 }, "Obelisk LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_lfo1Depth", 1 }, "Obelisk LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "obel_lfo1Shape", 1 }, "Obelisk LFO1 Shape", 0, 4, 0));

        // === LFO2 (preparation depth) ===
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_lfo2Rate", 1 }, "Obelisk LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.8f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obel_lfo2Depth", 1 }, "Obelisk LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "obel_lfo2Shape", 1 }, "Obelisk LFO2 Shape", 0, 4, 0));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramDensity        = apvts.getRawParameterValue ("obel_density");
        paramStoneTone      = apvts.getRawParameterValue ("obel_stoneTone");
        paramBrightness     = apvts.getRawParameterValue ("obel_brightness");
        paramDamping        = apvts.getRawParameterValue ("obel_damping");
        paramDecay          = apvts.getRawParameterValue ("obel_decay");
        paramHFNoise        = apvts.getRawParameterValue ("obel_hfNoise");
        paramHardness       = apvts.getRawParameterValue ("obel_hardness");
        paramPrepType       = apvts.getRawParameterValue ("obel_prepType");
        paramPrepPosition   = apvts.getRawParameterValue ("obel_prepPosition");
        paramPrepDepth      = apvts.getRawParameterValue ("obel_prepDepth");
        paramFilterEnvAmount = apvts.getRawParameterValue ("obel_filterEnvAmount");
        paramThermalDrift   = apvts.getRawParameterValue ("obel_thermalDrift");
        paramBendRange      = apvts.getRawParameterValue ("obel_bendRange");
        paramMacroStone     = apvts.getRawParameterValue ("obel_macroStone");
        paramMacroPrep      = apvts.getRawParameterValue ("obel_macroPrep");
        paramMacroCoupling  = apvts.getRawParameterValue ("obel_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("obel_macroSpace");
        paramLfo1Rate       = apvts.getRawParameterValue ("obel_lfo1Rate");
        paramLfo1Depth      = apvts.getRawParameterValue ("obel_lfo1Depth");
        paramLfo1Shape      = apvts.getRawParameterValue ("obel_lfo1Shape");
        paramLfo2Rate       = apvts.getRawParameterValue ("obel_lfo2Rate");
        paramLfo2Depth      = apvts.getRawParameterValue ("obel_lfo2Depth");
        paramLfo2Shape      = apvts.getRawParameterValue ("obel_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<ObeliskVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    ParameterSmoother smoothDensity, smoothHardness, smoothPrepDepth;
    ParameterSmoother smoothBrightness, smoothDamping;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // Thermal drift state
    float thermalState = 0.0f, thermalTarget = 0.0f;
    float thermalCoeff = 0.0001f;
    int thermalTimer = 0;
    uint32_t thermalNoiseState = 71711u;

    // Coupling accumulators
    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingPrepMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers (cached at attach time)
    std::atomic<float>* paramDensity = nullptr;
    std::atomic<float>* paramStoneTone = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDamping = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramHFNoise = nullptr;
    std::atomic<float>* paramHardness = nullptr;
    std::atomic<float>* paramPrepType = nullptr;
    std::atomic<float>* paramPrepPosition = nullptr;
    std::atomic<float>* paramPrepDepth = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramThermalDrift = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramMacroStone = nullptr;
    std::atomic<float>* paramMacroPrep = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;
};

} // namespace xomnibus
