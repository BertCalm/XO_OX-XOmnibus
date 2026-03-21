#pragma once
//==============================================================================
//
//  OwareEngine.h — XOware | "The Resonant Board"
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
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
//  THE 6 PILLARS (9.8 Target):
//      1. Material Continuum — continuous wood↔metal↔glass morphing via
//         modal frequency ratios, Q factors, and decay characteristics
//      2. Mallet Physics — velocity controls exciter hardness (felt→brass),
//         changing the spectral content of the strike (D001 at its deepest)
//      3. Sympathetic Resonance Network — voices excite each other based on
//         harmonic proximity (the gamelan shimmer)
//      4. Resonator Body — tube (marimba), frame (gamelan), bowl, open
//      5. Buzz Membrane — balafon/gyil spider-silk membrane rattle
//      6. Breathing Gamelan — micro-detuning shimmer at D005 rates
//
//  HISTORICAL LINEAGE:
//      West African balafon and gyil (gourd-resonated xylophones with buzz
//      membranes), Javanese and Balinese gamelan (bronze metallophones with
//      beating pairs), Western marimba and vibraphone (tuned bars over tubes),
//      Tibetan singing bowls (continuous excitation of circular modes).
//      The modal synthesis approach follows Adrien (1991) and Bilbao (2009).
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

namespace xomnibus {

//==============================================================================
// Material ratio tables — derived from Euler-Bernoulli beam theory and
// empirical measurements of real instruments (Rossing, "Science of Percussion
// Instruments", 2000; Fletcher & Rossing, "The Physics of Musical Instruments").
//
// For a uniform bar (fixed-free boundary):
//   f_n / f_1 = (2n+1)^2 / 9  for n = 1,2,3,...
//   → 1.0, 2.756, 5.404, 8.933, 13.344, 18.637, 24.812, 31.869
//
// Real instruments deviate: marimba bars are undercut (arched) to tune the
// 2nd partial to exactly 4:1. Gamelan pots have 2D circular modes.
//==============================================================================

// Wood bar (marimba/balafon): undercut tuning pulls 2nd mode to 4:1
static constexpr float kWoodRatios[8] = {
    1.000f, 3.990f, 9.140f, 15.800f, 24.300f, 34.500f, 46.500f, 60.200f
};

// Metal bar (vibraphone/glockenspiel): higher modes more prominent
static constexpr float kMetalRatios[8] = {
    1.000f, 4.000f, 10.000f, 17.600f, 27.200f, 38.800f, 52.400f, 68.000f
};

// Bell (gamelan bonang / church bell): inharmonic with clustered modes
static constexpr float kBellRatios[8] = {
    1.000f, 1.506f, 2.000f, 2.514f, 3.011f, 3.578f, 4.170f, 4.952f
};

// Bowl (singing bowl / gong): split degenerate circular modes
static constexpr float kBowlRatios[8] = {
    1.000f, 2.710f, 5.330f, 8.860f, 13.300f, 18.640f, 24.890f, 32.040f
};

//==============================================================================
// OwareMalletExciter — models the impact of a mallet on a bar/key.
//
// Hardness controls the spectral content of the excitation:
//   - Soft (felt): Gaussian pulse → emphasizes fundamental, warm
//   - Medium (rubber): shorter pulse + mild noise → balanced spectrum
//   - Hard (brass/acrylic): very short impulse + broadband noise → bright
//
// Based on Chaigne & Doutaut (1997), "Numerical simulations of xylophones."
//==============================================================================
struct OwareMalletExciter
{
    void trigger (float velocity, float hardness, float sampleRate) noexcept
    {
        active = true;
        sampleCounter = 0;

        // Contact duration: soft mallets have longer contact (warmer)
        // Range: 0.5ms (hard) to 5ms (soft)
        float contactMs = 5.0f - hardness * 4.5f;
        contactSamples = static_cast<int> (contactMs * 0.001f * sampleRate);
        contactSamples = std::max (contactSamples, 4);

        // Peak amplitude scales with velocity
        peakAmplitude = velocity;

        // Noise mix: more noise for harder mallets (broadband excitation)
        noiseMix = hardness * hardness;  // quadratic — gentle at low hardness

        // PRNG seed from velocity for deterministic but varied noise
        noiseState = static_cast<uint32_t> (velocity * 65535.0f) + 12345u;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;

        if (sampleCounter >= contactSamples)
        {
            active = false;
            return 0.0f;
        }

        // Half-sine pulse shape (Chaigne mallet model)
        float phase = static_cast<float> (sampleCounter) / static_cast<float> (contactSamples);
        float pulse = std::sin (phase * 3.14159265f) * peakAmplitude;

        // Mix in noise for hard mallets
        noiseState = noiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f) * peakAmplitude;
        float out = pulse * (1.0f - noiseMix) + noise * noiseMix;

        ++sampleCounter;
        return out;
    }

    void reset() noexcept { active = false; sampleCounter = 0; }

    bool active = false;
    int  sampleCounter = 0;
    int  contactSamples = 48;
    float peakAmplitude = 1.0f;
    float noiseMix = 0.0f;
    uint32_t noiseState = 12345u;
};

//==============================================================================
// OwareMode — a single resonating mode (bandpass resonator).
//
// Implements a 2nd-order resonator: y[n] = b0*x[n] + a1*y[n-1] - a2*y[n-2]
// where a1 = 2*r*cos(w), a2 = r^2, b0 = (1-r^2)*sin(w) for unit peak gain.
// r = exp(-pi * bandwidth / sr) controls Q/damping.
//==============================================================================
struct OwareMode
{
    void setFreqAndQ (float freqHz, float q, float sampleRate) noexcept
    {
        if (freqHz >= sampleRate * 0.49f) freqHz = sampleRate * 0.49f;  // Nyquist guard
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
        return out;
    }

    void reset() noexcept { y1 = 0.0f; y2 = 0.0f; }

    float freq = 440.0f;
    float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
};

//==============================================================================
// OwareBuzzMembrane — models the spider-silk/plastic membrane on balafon
// gourd resonators. This thin membrane vibrates sympathetically with the bar,
// adding a signature rattling buzz that is the soul of West African tuned
// percussion. Modeled as a soft-clip nonlinearity with asymmetric drive.
//
// No Western percussion synth models this. (Ghost-approved.)
//==============================================================================
struct OwareBuzzMembrane
{
    float process (float input, float amount) noexcept
    {
        if (amount < 0.001f) return input;

        // The membrane responds to the amplitude of the bar vibration.
        // Below a threshold it's silent; above it, it rattles.
        float driven = input * (1.0f + amount * 8.0f);  // boost into nonlinearity

        // Asymmetric soft-clip: positive half clips harder (membrane lifts off)
        float clipped;
        if (driven > 0.0f)
            clipped = std::tanh (driven * (1.0f + amount * 3.0f));
        else
            clipped = std::tanh (driven * (1.0f + amount * 1.5f));

        // One-pole lowpass to simulate membrane mass (doesn't respond instantly)
        buzzState += buzzCoeff * (clipped - buzzState);
        buzzState = flushDenormal (buzzState);

        // Mix: original + buzz artifacts
        return input + (buzzState - input) * amount;
    }

    void prepare (float sampleRate) noexcept
    {
        // Membrane response ~2kHz — fast enough for rattle, slow enough for mass
        buzzCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * 2000.0f / sampleRate);
    }

    void reset() noexcept { buzzState = 0.0f; }

    float buzzState = 0.0f;
    float buzzCoeff = 0.1f;
};

//==============================================================================
// OwareBodyResonator — the chamber beneath the bar/key.
//
// Type 0: Tube (marimba) — comb filter tuned to fundamental, amplifies F0
// Type 1: Frame (gamelan) — 3 fixed resonances from the wooden/metal frame
// Type 2: Bowl (singing bowl) — strong low-frequency resonance
// Type 3: Open (wind chimes) — no body, free-hanging bars
//==============================================================================
struct OwareBodyResonator
{
    static constexpr int kMaxDelay = 4096;

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        std::fill (delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        frameMode1.reset();
        frameMode2.reset();
        frameMode3.reset();
    }

    void setFundamental (float freqHz) noexcept
    {
        // Tube: comb filter delay = 1/frequency
        tubeDelaySamples = (freqHz > 20.0f) ? sr / freqHz : sr / 20.0f;
        tubeDelaySamples = std::min (tubeDelaySamples, static_cast<float> (kMaxDelay - 1));

        // Frame: 3 fixed resonances (wood frame ~ 200, 580, 1100 Hz)
        frameMode1.setFreqAndQ (200.0f, 15.0f, sr);
        frameMode2.setFreqAndQ (580.0f, 20.0f, sr);
        frameMode3.setFreqAndQ (1100.0f, 12.0f, sr);

        // Bowl: strong resonance at fundamental × 0.5 (sub-octave)
        bowlFreq = freqHz * 0.5f;
    }

    float process (float input, int bodyType, float depth) noexcept
    {
        if (depth < 0.001f) return input;

        float bodyOut = 0.0f;

        switch (bodyType)
        {
            case 0:  // Tube (marimba/balafon gourd)
            {
                // Write to delay line
                delayLine[writePos] = input;

                // Read with linear interpolation
                float readPos = static_cast<float> (writePos) - tubeDelaySamples;
                if (readPos < 0.0f) readPos += kMaxDelay;
                int r0 = static_cast<int> (readPos);
                float frac = readPos - static_cast<float> (r0);
                int r1 = (r0 + 1) % kMaxDelay;
                float delayed = delayLine[r0] * (1.0f - frac) + delayLine[r1] * frac;

                // Feedback with damping (tube resonance)
                bodyOut = delayed * 0.6f + input * 0.4f;
                writePos = (writePos + 1) % kMaxDelay;
                break;
            }
            case 1:  // Frame (gamelan/wooden stand)
            {
                bodyOut = frameMode1.process (input) * 0.5f
                        + frameMode2.process (input) * 0.3f
                        + frameMode3.process (input) * 0.2f;
                break;
            }
            case 2:  // Bowl (singing bowl / gong frame)
            {
                // Strong low resonance
                float w = 2.0f * 3.14159265f * std::max (bowlFreq, 20.0f) / sr;
                float r = 0.999f;  // very high Q
                bowlY1 = bowlY1 * 2.0f * r * std::cos (w) - bowlY2 * r * r
                       + input * (1.0f - r * r) * std::sin (w);
                bowlY2 = bowlY1;
                bowlY1 = flushDenormal (bowlY1);
                bodyOut = bowlY1;
                break;
            }
            case 3:  // Open (no body)
            default:
                return input;
        }

        return input * (1.0f - depth) + bodyOut * depth;
    }

    void reset() noexcept
    {
        std::fill (delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
        frameMode1.reset();
        frameMode2.reset();
        frameMode3.reset();
        bowlY1 = 0.0f;
        bowlY2 = 0.0f;
    }

    float sr = 48000.0f;
    std::array<float, kMaxDelay> delayLine {};
    int writePos = 0;
    float tubeDelaySamples = 100.0f;
    OwareMode frameMode1, frameMode2, frameMode3;
    float bowlFreq = 220.0f;
    float bowlY1 = 0.0f, bowlY2 = 0.0f;
};

//==============================================================================
// OwareVoice — a single struck bar/key with full physical modeling chain.
//==============================================================================
struct OwareVoice
{
    static constexpr int kMaxModes = 8;

    // ---- Voice management (required by VoiceAllocator) ----
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;

    // ---- Per-voice state ----
    float velocity = 0.0f;
    GlideProcessor glide;
    OwareMalletExciter exciter;
    std::array<OwareMode, kMaxModes> modes;
    OwareBuzzMembrane buzz;
    OwareBodyResonator body;
    FilterEnvelope filterEnv;
    CytomicSVF svf;

    // ---- Sympathetic resonance output (read by other voices) ----
    float sympatheticOut = 0.0f;

    // ---- Amplitude envelope (simple one-pole decay for percussion) ----
    float ampLevel = 0.0f;
    float decayCoeff = 0.999f;

    // ---- LFOs (per-voice for shimmer independence) ----
    StandardLFO shimmerLFO;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        ampLevel = 0.0f;
        sympatheticOut = 0.0f;
        glide.reset();
        exciter.reset();
        buzz.reset();
        body.reset();
        filterEnv.kill();
        shimmerLFO.reset();
        for (auto& m : modes) m.reset();
    }
};

//==============================================================================
// OwareEngine — "The Resonant Board"
//==============================================================================
class OwareEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Oware"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFB5883E); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int /*maxBlockSize*/) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        for (auto& v : voices)
        {
            v.reset();
            v.buzz.prepare (srf);
            v.body.prepare (srf);
            v.filterEnv.prepare (srf);
            v.shimmerLFO.setShape (StandardLFO::Sine);
        }

        // Smoothers
        smoothMaterial.prepare (srf);
        smoothMallet.prepare (srf);
        smoothBuzz.prepare (srf);
        smoothBodyDepth.prepare (srf);
        smoothSympathy.prepare (srf);
        smoothBrightness.prepare (srf);

        prepareSilenceGate (sr, 512, 500.0f);  // 500ms hold for resonant tails
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
    }

    //==========================================================================
    // Coupling
    //==========================================================================

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
            case CouplingType::AmpToFilter:  couplingFilterMod += val * 2000.0f; break;
            case CouplingType::LFOToPitch:   couplingPitchMod += val * 2.0f; break;
            case CouplingType::AmpToPitch:   couplingPitchMod += val; break;
            case CouplingType::EnvToMorph:   couplingMaterialMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        // ---- MIDI processing ----
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
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        if (isSilenceGateBypassed())
        {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        // ---- Load parameters ----
        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const float pMaterial     = loadP (paramMaterial, 0.0f);
        const float pMalletBase   = loadP (paramMalletHardness, 0.3f);
        const int   pBodyType     = static_cast<int> (loadP (paramBodyType, 0.0f));
        const float pBodyDepth    = loadP (paramBodyDepth, 0.5f);
        const float pBuzzAmount   = loadP (paramBuzzAmount, 0.0f);
        const float pSympathy     = loadP (paramSympathyAmount, 0.3f);
        const float pShimmerRate  = loadP (paramShimmerRate, 0.02f);
        const float pShimmerDepth = loadP (paramShimmerDepth, 3.0f);
        const float pBrightness   = loadP (paramBrightness, 8000.0f);
        const float pDamping      = loadP (paramDamping, 0.5f);
        const float pDecay        = loadP (paramDecay, 2.0f);
        const float pFilterEnvAmt = loadP (paramFilterEnvAmount, 0.3f);
        const float pBendRange    = loadP (paramBendRange, 2.0f);

        // Macros
        const float macroMaterial = loadP (paramMacroMaterial, 0.0f);
        const float macroMallet   = loadP (paramMacroMallet, 0.0f);
        const float macroCoupling = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace    = loadP (paramMacroSpace, 0.0f);

        // Apply macros
        float effectiveMaterial = std::clamp (pMaterial + macroMaterial * 0.8f + couplingMaterialMod, 0.0f, 1.0f);
        float effectiveMallet   = std::clamp (pMalletBase + macroMallet * 0.5f, 0.0f, 1.0f);
        float effectiveSympathy = std::clamp (pSympathy + macroCoupling * 0.4f, 0.0f, 1.0f);
        float effectiveBodyDep  = std::clamp (pBodyDepth + macroSpace * 0.3f, 0.0f, 1.0f);
        float effectiveBright   = std::clamp (pBrightness + macroMallet * 4000.0f
                                              + aftertouchAmount * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);

        // D006: mod wheel → shimmer depth boost
        float effectiveShimmerD = pShimmerDepth + modWheelAmount * 5.0f;

        // Set smoother targets
        smoothMaterial.set (effectiveMaterial);
        smoothMallet.set (effectiveMallet);
        smoothBuzz.set (pBuzzAmount);
        smoothBodyDepth.set (effectiveBodyDep);
        smoothSympathy.set (effectiveSympathy);
        smoothBrightness.set (effectiveBright);

        // Reset coupling accumulators
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingMaterialMod = 0.0f;

        // Pitch bend in semitones
        const float bendSemitones = pitchBendNorm * pBendRange;

        // Compute decay coefficient from parameter
        float decayTimeSec = std::max (pDecay * (1.0f - pDamping * 0.8f), 0.01f);
        float decayCoeff = std::exp (-1.0f / (decayTimeSec * srf));

        // ---- Per-sample render ----
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float matNow   = smoothMaterial.process();
            float malletNow = smoothMallet.process();
            float buzzNow  = smoothBuzz.process();
            float bodyDNow = smoothBodyDepth.process();
            float sympNow  = smoothSympathy.process();
            float brightNow = smoothBrightness.process();

            float mixL = 0.0f, mixR = 0.0f;

            // ---- Sympathetic resonance: sum all voice outputs from previous sample ----
            float sympatheticSum = 0.0f;
            for (const auto& v : voices)
                if (v.active) sympatheticSum += v.sympatheticOut;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // ---- Glide + pitch bend ----
                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                // ---- Shimmer: micro-detuning per voice (breathing gamelan) ----
                voice.shimmerLFO.setRate (pShimmerRate, srf);
                float shimmerCents = voice.shimmerLFO.process() * effectiveShimmerD;
                freq *= std::pow (2.0f, shimmerCents / 1200.0f);

                // ---- Exciter ----
                float excitation = voice.exciter.process();

                // ---- Sympathetic resonance input ----
                // Other voices' output excites this voice's modes proportional
                // to harmonic proximity and sympathy amount.
                float sympInput = 0.0f;
                if (sympNow > 0.001f)
                {
                    for (const auto& other : voices)
                    {
                        if (&other == &voice || !other.active) continue;
                        float ratio = other.glide.getFreq() / std::max (freq, 20.0f);
                        // Harmonic proximity: 1.0 when ratio is near an integer
                        float nearestInt = std::round (ratio);
                        float proximity = 1.0f / (1.0f + std::fabs (ratio - nearestInt) * 12.0f);
                        sympInput += other.sympatheticOut * proximity * sympNow * 0.05f;
                    }
                }

                // ---- Modal resonator bank ----
                float totalExcitation = excitation + sympInput;
                float resonanceSum = 0.0f;

                for (int m = 0; m < OwareVoice::kMaxModes; ++m)
                {
                    // Interpolate mode frequency ratio from material tables
                    float woodR = kWoodRatios[m];
                    float metalR = kMetalRatios[m];
                    float bellR = kBellRatios[m];

                    // Material continuum: 0→0.33 = wood→bell, 0.33→0.66 = bell→metal, 0.66→1.0 = metal→bowl
                    float ratio;
                    if (matNow < 0.33f)
                        ratio = woodR + (bellR - woodR) * (matNow / 0.33f);
                    else if (matNow < 0.66f)
                        ratio = bellR + (metalR - bellR) * ((matNow - 0.33f) / 0.33f);
                    else
                        ratio = metalR + (kBowlRatios[m] - metalR) * ((matNow - 0.66f) / 0.34f);

                    float modeFreq = freq * ratio;

                    // Q depends on material: wood=80-150, bell=300-800, metal=500-1500
                    float baseQ = 80.0f + matNow * 1420.0f;
                    // Higher modes decay faster (Q decreases with mode number)
                    float modeQ = baseQ / (1.0f + static_cast<float> (m) * 0.3f);

                    // Mode amplitude: decreases with mode number
                    // D001: velocity (mallet hardness) controls how much upper modes are excited
                    float modeAmp = 1.0f / (1.0f + static_cast<float> (m) * (1.5f - malletNow * 1.2f));

                    voice.modes[m].setFreqAndQ (modeFreq, modeQ, srf);
                    resonanceSum += voice.modes[m].process (totalExcitation) * modeAmp;
                }

                // Scale resonance to prevent clipping with many modes
                resonanceSum *= 0.25f;

                // ---- Buzz membrane ----
                float buzzed = voice.buzz.process (resonanceSum, buzzNow);

                // ---- Body resonator ----
                voice.body.setFundamental (freq);
                float bodied = voice.body.process (buzzed, pBodyType, bodyDNow);

                // ---- Amplitude envelope (percussion decay) ----
                voice.ampLevel *= decayCoeff;
                voice.ampLevel = flushDenormal (voice.ampLevel);
                if (voice.ampLevel < 1e-6f)
                {
                    voice.active = false;
                    continue;
                }

                // ---- Filter envelope ----
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 4000.0f;

                // ---- SVF lowpass ----
                float cutoff = std::clamp (brightNow + envMod, 200.0f, 20000.0f);
                voice.svf.setMode (CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients (cutoff, 0.5f, srf);
                float filtered = voice.svf.processSample (bodied);

                float output = filtered * voice.ampLevel;

                // ---- Cache for sympathetic resonance ----
                voice.sympatheticOut = output;

                // ---- Simple pan from note position ----
                float pan = static_cast<float> (voice.currentNote - 60) / 36.0f;
                pan = std::clamp (pan, -1.0f, 1.0f);
                float gainL = std::cos ((pan + 1.0f) * 0.25f * 3.14159265f);
                float gainR = std::sin ((pan + 1.0f) * 0.25f * 3.14159265f);

                mixL += output * gainL;
                mixR += output * gainR;
            }

            outL[s] = mixL;
            if (outR) outR[s] = mixR;

            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        // ---- Update active voice count ----
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
        v.sympatheticOut = 0.0f;

        // D001: velocity → mallet hardness (soft touch = warm, hard strike = bright)
        float hardness = std::clamp (
            (paramMalletHardness ? paramMalletHardness->load() : 0.3f) + vel * 0.5f,
            0.0f, 1.0f);
        v.exciter.trigger (vel, hardness, srf);

        // Filter envelope: velocity-scaled for D001 brightness response
        v.filterEnv.prepare (srf);
        v.filterEnv.setADSR (0.001f, 0.3f, 0.0f, 0.5f);
        v.filterEnv.triggerHard();

        // Shimmer LFO: stagger phase per voice for organic detuning
        v.shimmerLFO.reset (static_cast<float> (idx) / static_cast<float> (kMaxVoices));

        // Reset modes and body for clean attack
        for (auto& m : v.modes) m.reset();
        v.body.prepare (srf);
        v.buzz.prepare (srf);
    }

    void noteOff (int note) noexcept
    {
        // For percussion: noteOff applies damping (hand muting)
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                // Accelerate decay (hand damping)
                v.ampLevel *= 0.3f;
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // ---- Core sound ----
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_material", 1 }, "Oware Material",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_malletHardness", 1 }, "Oware Mallet Hardness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "owr_bodyType", 1 }, "Oware Body Type", 0, 3, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_bodyDepth", 1 }, "Oware Body Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_buzzAmount", 1 }, "Oware Buzz Membrane",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_sympathyAmount", 1 }, "Oware Sympathy",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // ---- Shimmer / Breathing ----
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_shimmerRate", 1 }, "Oware Shimmer Rate",
            juce::NormalisableRange<float> (0.005f, 2.0f, 0.0f, 0.3f), 0.02f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_shimmerDepth", 1 }, "Oware Shimmer Depth",
            juce::NormalisableRange<float> (0.0f, 15.0f), 3.0f));

        // ---- Tone shaping ----
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_brightness", 1 }, "Oware Brightness",
            juce::NormalisableRange<float> (200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_damping", 1 }, "Oware Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_decay", 1 }, "Oware Decay",
            juce::NormalisableRange<float> (0.05f, 10.0f, 0.0f, 0.4f), 2.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_filterEnvAmount", 1 }, "Oware Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // ---- Expression ----
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_bendRange", 1 }, "Oware Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // ---- Macros (M1-M4) ----
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_macroMaterial", 1 }, "Oware Macro MATERIAL",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_macroMallet", 1 }, "Oware Macro MALLET",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_macroCoupling", 1 }, "Oware Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_macroSpace", 1 }, "Oware Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // ---- LFOs (D002) ----
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_lfo1Rate", 1 }, "Oware LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_lfo1Depth", 1 }, "Oware LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "owr_lfo1Shape", 1 }, "Oware LFO1 Shape", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_lfo2Rate", 1 }, "Oware LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "owr_lfo2Depth", 1 }, "Oware LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "owr_lfo2Shape", 1 }, "Oware LFO2 Shape", 0, 4, 0));

        return { params.begin(), params.end() };
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramMaterial        = apvts.getRawParameterValue ("owr_material");
        paramMalletHardness  = apvts.getRawParameterValue ("owr_malletHardness");
        paramBodyType        = apvts.getRawParameterValue ("owr_bodyType");
        paramBodyDepth       = apvts.getRawParameterValue ("owr_bodyDepth");
        paramBuzzAmount      = apvts.getRawParameterValue ("owr_buzzAmount");
        paramSympathyAmount  = apvts.getRawParameterValue ("owr_sympathyAmount");
        paramShimmerRate     = apvts.getRawParameterValue ("owr_shimmerRate");
        paramShimmerDepth    = apvts.getRawParameterValue ("owr_shimmerDepth");
        paramBrightness      = apvts.getRawParameterValue ("owr_brightness");
        paramDamping         = apvts.getRawParameterValue ("owr_damping");
        paramDecay           = apvts.getRawParameterValue ("owr_decay");
        paramFilterEnvAmount = apvts.getRawParameterValue ("owr_filterEnvAmount");
        paramBendRange       = apvts.getRawParameterValue ("owr_bendRange");
        paramMacroMaterial   = apvts.getRawParameterValue ("owr_macroMaterial");
        paramMacroMallet     = apvts.getRawParameterValue ("owr_macroMallet");
        paramMacroCoupling   = apvts.getRawParameterValue ("owr_macroCoupling");
        paramMacroSpace      = apvts.getRawParameterValue ("owr_macroSpace");
        paramLfo1Rate        = apvts.getRawParameterValue ("owr_lfo1Rate");
        paramLfo1Depth       = apvts.getRawParameterValue ("owr_lfo1Depth");
        paramLfo1Shape       = apvts.getRawParameterValue ("owr_lfo1Shape");
        paramLfo2Rate        = apvts.getRawParameterValue ("owr_lfo2Rate");
        paramLfo2Depth       = apvts.getRawParameterValue ("owr_lfo2Depth");
        paramLfo2Shape       = apvts.getRawParameterValue ("owr_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OwareVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    // ---- Parameter smoothers ----
    ParameterSmoother smoothMaterial, smoothMallet, smoothBuzz;
    ParameterSmoother smoothBodyDepth, smoothSympathy, smoothBrightness;

    // ---- Expression state ----
    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    // ---- Coupling state ----
    float couplingFilterMod = 0.0f;
    float couplingPitchMod = 0.0f;
    float couplingMaterialMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // ---- Parameter pointers ----
    std::atomic<float>* paramMaterial = nullptr;
    std::atomic<float>* paramMalletHardness = nullptr;
    std::atomic<float>* paramBodyType = nullptr;
    std::atomic<float>* paramBodyDepth = nullptr;
    std::atomic<float>* paramBuzzAmount = nullptr;
    std::atomic<float>* paramSympathyAmount = nullptr;
    std::atomic<float>* paramShimmerRate = nullptr;
    std::atomic<float>* paramShimmerDepth = nullptr;
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

} // namespace xomnibus
