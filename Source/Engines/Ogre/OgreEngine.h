#pragma once
//==============================================================================
//
//  OgreEngine.h — XOgre | "The Root Cellar"
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOgre is the deep earth beneath the kitchen — root vegetables grown in
//      heavy clay soil, dense and concentrated through slow underground pressure.
//      It exists below the glamorous surface. It is what the entire structure
//      rests on. An earthquake bass. The lowest frequency content in the fleet.
//
//  ENGINE CONCEPT:
//      A massive sub-bass synthesizer built on two oscillators (sine + triangle)
//      with thick tanh saturation, a sub-harmonic generator (octave-down from
//      input), and a body resonance filter. The sub extends below the fundamental
//      via the rootDepth parameter. Gravitational coupling pulls other engines'
//      pitches toward the bass fundamental via MIDI-domain note tracking.
//
//  CELLAR QUAD ROLE: Sub Bass / Root Vegetables
//      The foundation of the foundation. Where XOlate has warmth and XOaken has
//      wood, XOgre has weight. Pure low-frequency presence that you feel in your
//      chest before you hear it in your ears.
//
//  DSP ARCHITECTURE:
//      1. Dual oscillator (sine + triangle via PolyBLEP) with mix crossfade
//      2. Sub-harmonic generator — octave-down from osc output (frequency halving)
//      3. Thick tanh saturation with drive control
//      4. Body resonance filter (CytomicSVF LP with resonance)
//      5. MIDI-domain gravitational coupling (decision C1: no FFT)
//      6. rootDepth: extends sub content below fundamental via sub-octave mix
//      7. Tectonic LFO: extremely slow pitch modulation (continental drift)
//
//  GRAVITATIONAL COUPLING:
//      {ogre_gravity} broadcasts this engine's pitch + mass to coupled engines.
//      Mass accumulates with note duration (held bass note = stronger pull).
//      MIDI-domain only — no FFT (CPU strategy decision C1, ~3.1% CPU target).
//
//  Accent: Deep Earth Brown #4A2C0A (from Visionary doc)
//  Parameter prefix: ogre_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PolyBLEP.h"
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
// Sub-harmonic generator — frequency divider producing an octave-down signal
// from the input oscillator. Uses a simple toggle flip-flop on zero crossings
// to halve the frequency, then lowpass-filters the result for smoothness.
//==============================================================================
struct OgreSubHarmonicGen
{
    float process (float input) noexcept
    {
        // Detect zero crossings and toggle
        if ((input > 0.0f && lastSample <= 0.0f) || (input < 0.0f && lastSample >= 0.0f))
            toggleState = !toggleState;
        lastSample = input;

        // Sub-octave square wave from toggle, then smooth with one-pole LP
        float subRaw = toggleState ? 1.0f : -1.0f;
        subFiltered += subCoeff * (subRaw - subFiltered);
        return flushDenormal (subFiltered);
    }

    void prepare (float sampleRate) noexcept
    {
        // One-pole LP at ~80 Hz to smooth the sub-octave square into something rounder
        float fc = 80.0f;
        subCoeff = 1.0f - std::exp (-2.0f * 3.14159265f * fc / sampleRate);
    }

    void reset() noexcept
    {
        lastSample = 0.0f;
        toggleState = false;
        subFiltered = 0.0f;
    }

    float lastSample = 0.0f;
    bool toggleState = false;
    float subFiltered = 0.0f;
    float subCoeff = 0.1f;
};

//==============================================================================
// OgreVoice — Sub bass voice with dual osc + sub-harmonic + saturation
//==============================================================================
struct OgreVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 36;  // bass range default
    float velocity = 0.0f;

    GlideProcessor glide;
    PolyBLEP oscSine;      // used as sine
    PolyBLEP oscTri;       // used as triangle
    OgreSubHarmonicGen subGen;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF bodyFilter;
    CytomicSVF subFilter;  // LP for sub content shaping
    StandardLFO lfo1, lfo2;
    StandardLFO tectonicLFO;  // ultra-slow pitch modulation

    // Gravitational mass accumulator — grows during sustain
    float gravityMass = 0.0f;
    float noteHeldTime = 0.0f;  // in seconds

    // Cached per-voice values
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        gravityMass = 0.0f;
        noteHeldTime = 0.0f;
        glide.reset();
        oscSine.reset();
        oscTri.reset();
        subGen.reset();
        ampEnv.kill();
        filterEnv.kill();
        bodyFilter.reset();
        subFilter.reset();
        lfo1.reset();
        lfo2.reset();
        tectonicLFO.reset();
    }
};

//==============================================================================
// OgreEngine — "The Root Cellar" | Sub Bass with Gravitational Coupling
//==============================================================================
class OgreEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Ogre"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF4A2C0A); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].subGen.prepare (srf);
            voices[i].ampEnv.prepare (srf);
            voices[i].filterEnv.prepare (srf);
            voices[i].tectonicLFO.setShape (StandardLFO::Sine);
            // Stagger tectonic LFO phases for organic feel
            voices[i].tectonicLFO.reset (static_cast<float> (i) / static_cast<float> (kMaxVoices));
        }

        smoothDrive.prepare (srf);
        smoothOscMix.prepare (srf);
        smoothRootDepth.prepare (srf);
        smoothBodyRes.prepare (srf);
        smoothBrightness.prepare (srf);
        smoothDensity.prepare (srf);

        prepareSilenceGate (sr, maxBlockSize, 300.0f);  // bass sustain tail
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
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
            default: break;
        }
    }

    //==========================================================================
    // Render
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
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

        if (isSilenceGateBypassed()) {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const float pDrive      = loadP (paramDrive, 0.3f);
        const float pOscMix     = loadP (paramOscMix, 0.0f);     // 0=sine, 1=triangle
        const float pRootDepth  = loadP (paramRootDepth, 0.5f);
        const float pBodyRes    = loadP (paramBodyResonance, 0.3f);
        const float pBrightness = loadP (paramBrightness, 2000.0f);
        const float pDensity    = loadP (paramDensity, 0.5f);
        const float pSoil       = loadP (paramSoil, 0.0f);
        const float pTectonic   = loadP (paramTectonicRate, 0.01f);
        const float pTecDep     = loadP (paramTectonicDepth, 0.3f);
        const float pGlide      = loadP (paramGlide, 0.0f);
        const float pAttack     = loadP (paramAttack, 0.01f);
        const float pDecay      = loadP (paramDecay, 0.5f);
        const float pSustain    = loadP (paramSustain, 0.8f);
        const float pRelease    = loadP (paramRelease, 0.3f);
        const float pFiltEnvAmt = loadP (paramFilterEnvAmount, 0.4f);
        const float pBendRange  = loadP (paramBendRange, 2.0f);
        const float pGravity    = loadP (paramGravity, 0.5f);

        const float macroChar   = loadP (paramMacroCharacter, 0.0f);
        const float macroMove   = loadP (paramMacroMovement, 0.0f);
        const float macroCoup   = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace  = loadP (paramMacroSpace, 0.0f);

        // D006: mod wheel -> drive intensity, aftertouch -> body resonance
        float effectiveDrive = std::clamp (pDrive + macroChar * 0.5f + modWheelAmount * 0.4f, 0.0f, 1.0f);
        float effectiveBodyRes = std::clamp (pBodyRes + macroSpace * 0.3f + aftertouchAmount * 0.3f, 0.0f, 1.0f);
        float effectiveBright = std::clamp (pBrightness + macroChar * 3000.0f + couplingFilterMod, 100.0f, 20000.0f);

        smoothDrive.set (effectiveDrive);
        smoothOscMix.set (pOscMix);
        smoothRootDepth.set (pRootDepth);
        smoothBodyRes.set (effectiveBodyRes);
        smoothBrightness.set (effectiveBright);
        smoothDensity.set (pDensity);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Soil filter character: clay=LP resonant, sandy=wider, rocky=HP blend
        // Maps pSoil [0=clay, 0.5=sandy, 1.0=rocky]

        // LFO params
        const float lfo1Rate  = loadP (paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP (paramLfo1Depth, 0.0f);
        const int   lfo1Shape = static_cast<int> (loadP (paramLfo1Shape, 0.0f));
        const float lfo2Rate  = loadP (paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP (paramLfo2Depth, 0.0f);
        const int   lfo2Shape = static_cast<int> (loadP (paramLfo2Shape, 0.0f));

        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);
            voice.tectonicLFO.setRate (pTectonic, srf);
            voice.glide.setTime (pGlide, srf);
            voice.ampEnv.setADSR (pAttack, pDecay, pSustain, pRelease);
        }

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        const float dtSec = 1.0f / srf;  // for mass accumulation

        for (int s = 0; s < numSamples; ++s)
        {
            float driveNow  = smoothDrive.process();
            float mixNow    = smoothOscMix.process();
            float rootDNow  = smoothRootDepth.process();
            float bodyRNow  = smoothBodyRes.process();
            float brightNow = smoothBrightness.process();
            float densNow   = smoothDensity.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                // Tectonic LFO: extremely slow pitch drift (±pTecDep cents at most)
                float tecMod = voice.tectonicLFO.process() * pTecDep;
                freq *= fastPow2 (tecMod / 1200.0f);

                // LFO modulations
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // Set oscillator frequencies
                voice.oscSine.setFrequency (freq, srf);
                voice.oscSine.setWaveform (PolyBLEP::Waveform::Sine);
                voice.oscTri.setFrequency (freq, srf);
                voice.oscTri.setWaveform (PolyBLEP::Waveform::Triangle);

                // Dual oscillator mix
                float sineSample = voice.oscSine.processSample();
                float triSample  = voice.oscTri.processSample();
                float oscOut = sineSample * (1.0f - mixNow) + triSample * mixNow;

                // Sub-harmonic generator (octave down)
                float subSample = voice.subGen.process (oscOut);

                // Mix sub with main based on rootDepth
                float combined = oscOut * (1.0f - rootDNow * 0.6f) + subSample * rootDNow;

                // Density: adds sub-frequency content below hearing (infrasound presence)
                // Implemented as very low frequency emphasis on the sub
                voice.subFilter.setMode (CytomicSVF::Mode::LowPass);
                voice.subFilter.setCoefficients (40.0f + densNow * 20.0f, 0.3f + densNow * 0.4f, srf);
                float subEmphasis = voice.subFilter.processSample (combined) * densNow * 0.5f;
                combined += subEmphasis;

                // Tanh saturation — thick, earthy
                float driveAmount = 1.0f + driveNow * 8.0f;
                float saturated = fastTanh (combined * driveAmount) / fastTanh (driveAmount);

                // D001: velocity shapes saturation intensity and brightness
                float velBright = brightNow + voice.velocity * 3000.0f;
                // LFO1 -> brightness
                velBright = std::clamp (velBright + lfo1Val * 2000.0f, 100.0f, 20000.0f);

                // Body resonance filter — soil character determines filter mode, base cutoff, and Q.
                // Clay (0) = tight LP, Sandy (0.5) = wider LP, Rocky (1.0) = BandPass notch character.
                // The filter envelope then modulates the soil-derived cutoff.
                // Both soil character AND envelope affect the single filter pass (D004 fix).
                float soilQ = 0.2f + bodyRNow * 0.6f;

                // Filter envelope — compute first so it feeds into soil cutoff calculation
                float envMod = voice.filterEnv.process() * pFiltEnvAmt * 3000.0f;
                float lfo2FilterMod = lfo2Val * 1500.0f;

                float filtered;
                if (pSoil < 0.5f)
                {
                    voice.bodyFilter.setMode (CytomicSVF::Mode::LowPass);
                    // Soil sets base character: clay = darker, sandy = brighter
                    float soilBaseCutoff = velBright * (0.6f + pSoil * 0.8f);
                    // Envelope and LFO2 modulate on top of soil base
                    float finalCutoff = std::clamp (soilBaseCutoff + envMod + lfo2FilterMod,
                                                    100.0f, 20000.0f);
                    voice.bodyFilter.setCoefficients (finalCutoff, soilQ, srf);
                }
                else
                {
                    // Rocky: BandPass for notched character — soil pSoil > 0.5 means rock
                    voice.bodyFilter.setMode (CytomicSVF::Mode::BandPass);
                    float soilBaseCutoff = velBright * (0.3f + (pSoil - 0.5f) * 0.4f);
                    float finalCutoff = std::clamp (soilBaseCutoff + envMod + lfo2FilterMod,
                                                    100.0f, 20000.0f);
                    voice.bodyFilter.setCoefficients (finalCutoff, soilQ + 0.2f, srf);
                }
                // Single filter pass — soil character AND envelope both shape the result
                filtered = voice.bodyFilter.processSample (saturated);

                // Amplitude envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive()) { voice.active = false; continue; }

                // Gravitational mass accumulation — grows during note sustain
                voice.noteHeldTime += dtSec;
                float massAccumRate = 0.1f;  // ~10 seconds to full mass
                voice.gravityMass = std::min (1.0f, voice.gravityMass + dtSec * massAccumRate * pGravity);

                float output = filtered * ampLevel * voice.velocity;

                mixL += output * voice.panL;
                mixR += output * voice.panR;
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

        v.reset();
        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo (freq);
        v.gravityMass = 0.0f;
        v.noteHeldTime = 0.0f;

        v.subGen.prepare (srf);
        v.ampEnv.prepare (srf);
        v.filterEnv.prepare (srf);

        float attack = paramAttack ? paramAttack->load() : 0.01f;
        float decay = paramDecay ? paramDecay->load() : 0.5f;
        float sustain = paramSustain ? paramSustain->load() : 0.8f;
        float release = paramRelease ? paramRelease->load() : 0.3f;
        v.ampEnv.setADSR (attack, decay, sustain, release);
        v.ampEnv.triggerHard();
        v.filterEnv.setADSR (0.001f, 0.2f, 0.0f, 0.3f);
        v.filterEnv.triggerHard();

        // Stereo spread — subtle for bass
        float pan = 0.5f + (static_cast<float> (idx) - 3.5f) * 0.03f;
        v.panL = std::cos (pan * 1.5707963f);
        v.panR = std::sin (pan * 1.5707963f);
    }

    void noteOff (int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.ampEnv.release();
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 30 total
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

        // Core oscillator
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_oscMix", 1 }, "Ogre Osc Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));  // 0=sine, 1=triangle
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_drive", 1 }, "Ogre Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_rootDepth", 1 }, "Ogre Root Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Body / Character
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_bodyResonance", 1 }, "Ogre Body Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_brightness", 1 }, "Ogre Brightness",
            juce::NormalisableRange<float> (100.0f, 20000.0f, 0.0f, 0.3f), 2000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_density", 1 }, "Ogre Density",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_soil", 1 }, "Ogre Soil",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));  // 0=clay, 0.5=sandy, 1=rocky

        // Tectonic
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_tectonicRate", 1 }, "Ogre Tectonic Rate",
            juce::NormalisableRange<float> (0.005f, 0.5f, 0.0f, 0.3f), 0.01f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_tectonicDepth", 1 }, "Ogre Tectonic Depth",
            juce::NormalisableRange<float> (0.0f, 20.0f), 3.0f));  // in cents

        // Envelope
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_attack", 1 }, "Ogre Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f), 0.01f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_decay", 1 }, "Ogre Decay",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_sustain", 1 }, "Ogre Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_release", 1 }, "Ogre Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_filterEnvAmount", 1 }, "Ogre Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        // Performance
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_glide", 1 }, "Ogre Glide",
            juce::NormalisableRange<float> (0.0f, 5.0f, 0.0f, 0.3f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_bendRange", 1 }, "Ogre Pitch Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // Gravitational coupling
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_gravity", 1 }, "Ogre Gravity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_macroCharacter", 1 }, "Ogre Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_macroMovement", 1 }, "Ogre Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_macroCoupling", 1 }, "Ogre Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_macroSpace", 1 }, "Ogre Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_lfo1Rate", 1 }, "Ogre LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_lfo1Depth", 1 }, "Ogre LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "ogre_lfo1Shape", 1 }, "Ogre LFO1 Shape", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_lfo2Rate", 1 }, "Ogre LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "ogre_lfo2Depth", 1 }, "Ogre LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "ogre_lfo2Shape", 1 }, "Ogre LFO2 Shape", 0, 4, 0));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramOscMix          = apvts.getRawParameterValue ("ogre_oscMix");
        paramDrive           = apvts.getRawParameterValue ("ogre_drive");
        paramRootDepth       = apvts.getRawParameterValue ("ogre_rootDepth");
        paramBodyResonance   = apvts.getRawParameterValue ("ogre_bodyResonance");
        paramBrightness      = apvts.getRawParameterValue ("ogre_brightness");
        paramDensity         = apvts.getRawParameterValue ("ogre_density");
        paramSoil            = apvts.getRawParameterValue ("ogre_soil");
        paramTectonicRate    = apvts.getRawParameterValue ("ogre_tectonicRate");
        paramTectonicDepth   = apvts.getRawParameterValue ("ogre_tectonicDepth");
        paramAttack          = apvts.getRawParameterValue ("ogre_attack");
        paramDecay           = apvts.getRawParameterValue ("ogre_decay");
        paramSustain         = apvts.getRawParameterValue ("ogre_sustain");
        paramRelease         = apvts.getRawParameterValue ("ogre_release");
        paramFilterEnvAmount = apvts.getRawParameterValue ("ogre_filterEnvAmount");
        paramGlide           = apvts.getRawParameterValue ("ogre_glide");
        paramBendRange       = apvts.getRawParameterValue ("ogre_bendRange");
        paramGravity         = apvts.getRawParameterValue ("ogre_gravity");
        paramMacroCharacter  = apvts.getRawParameterValue ("ogre_macroCharacter");
        paramMacroMovement   = apvts.getRawParameterValue ("ogre_macroMovement");
        paramMacroCoupling   = apvts.getRawParameterValue ("ogre_macroCoupling");
        paramMacroSpace      = apvts.getRawParameterValue ("ogre_macroSpace");
        paramLfo1Rate        = apvts.getRawParameterValue ("ogre_lfo1Rate");
        paramLfo1Depth       = apvts.getRawParameterValue ("ogre_lfo1Depth");
        paramLfo1Shape       = apvts.getRawParameterValue ("ogre_lfo1Shape");
        paramLfo2Rate        = apvts.getRawParameterValue ("ogre_lfo2Rate");
        paramLfo2Depth       = apvts.getRawParameterValue ("ogre_lfo2Depth");
        paramLfo2Shape       = apvts.getRawParameterValue ("ogre_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OgreVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    ParameterSmoother smoothDrive, smoothOscMix, smoothRootDepth;
    ParameterSmoother smoothBodyRes, smoothBrightness, smoothDensity;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramOscMix = nullptr;
    std::atomic<float>* paramDrive = nullptr;
    std::atomic<float>* paramRootDepth = nullptr;
    std::atomic<float>* paramBodyResonance = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDensity = nullptr;
    std::atomic<float>* paramSoil = nullptr;
    std::atomic<float>* paramTectonicRate = nullptr;
    std::atomic<float>* paramTectonicDepth = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramGlide = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramGravity = nullptr;
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
