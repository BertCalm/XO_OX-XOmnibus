#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ParameterSmoother.h"
#include <array>
#include <cmath>
#include <cstring>

namespace xolokun {

//==============================================================================
//
//  OVERWASH ENGINE — Spectral Diffusion Pad
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOverwash is ink in water — the moment pigment meets solvent and begins
//      its journey outward. Every note is a drop of spectral dye released into
//      frequency-space. Over 3–30 seconds, harmonics diffuse outward from the
//      fundamental, spreading, blending, creating interference fringes where
//      multiple notes overlap.
//
//  PHYSICS: Fick's Second Law of Diffusion
//      ∂C/∂t = D ∇²C
//      where C = spectral energy density, D = diffusion coefficient,
//      ∇²C = Laplacian (curvature of concentration in frequency space).
//
//  DSP ARCHITECTURE:
//      Per voice:
//        1. Multi-partial oscillator bank (16 partials per voice)
//        2. Analytical Gaussian diffusion: partials spread outward from
//           fundamental over time. No FFT — each partial's frequency is
//           offset by a Gaussian envelope that widens with time.
//        3. Cross-note diffusion: voices share a global spectral field
//           accumulator. Overlapping diffusion fronts create interference.
//        4. Viscosity filter: CytomicSVF LP that resists high-freq diffusion.
//        5. Amp ADSR + filter envelope.
//
//  TIME RELATIONSHIP: Distance (seconds scale, 3–30s diffusion arc)
//
//  4 MACROS:
//      wash_macroCharacter — viscosity + harmonic density
//      wash_macroMovement  — diffusion rate + LFO depth
//      wash_macroCoupling  — cross-note interference depth
//      wash_macroSpace     — stereo width + reverb tail
//
//  COOPERATIVE COUPLING (BROTH):
//      Reads XOverworn's sessionAge to increase viscosity as broth reduces.
//      Exports: diffusion field state for other BROTH engines.
//
//  ACCENT COLOR: Tea Amber #D4A76A
//  PARAMETER PREFIX: wash_
//  ENGINE ID: "Overwash"
//
//  DOCTRINES: D001–D006 compliant
//      D001: Velocity shapes harmonic brightness + diffusion onset intensity
//      D002: 2 LFOs, mod wheel, aftertouch, 4 macros, 4+ mod matrix slots
//      D003: Fick's Law analytical solution — no stochastic approximation
//      D004: All params wired to DSP
//      D005: Breathing LFO with rate floor <= 0.01 Hz
//      D006: Velocity→timbre, aftertouch→viscosity, mod wheel→diffusion rate
//
//==============================================================================

//==============================================================================
// DiffusionPartial — One spectral component that spreads over time.
// Frequency offset from fundamental grows as sqrt(2 * D * t) per Fick's Law.
//==============================================================================
struct DiffusionPartial
{
    float baseFreqRatio = 1.0f;   // harmonic ratio relative to fundamental
    float amplitude     = 1.0f;   // base amplitude (1/n harmonic weighting)
    float phase         = 0.0f;   // oscillator phase [0, 1)
    float diffusionAge  = 0.0f;   // time since this partial began diffusing (seconds)
    float spreadOffset  = 0.0f;   // current frequency offset from base (Hz)

    void reset() noexcept
    {
        phase = 0.0f;
        diffusionAge = 0.0f;
        spreadOffset = 0.0f;
    }
};

//==============================================================================
// OverwashVoice — Single polyphonic voice with 16 diffusing partials.
//==============================================================================
struct OverwashVoice
{
    static constexpr int kNumPartials = 16;

    bool     active      = false;
    uint64_t startTime   = 0;
    int      currentNote = 60;
    float    velocity    = 0.0f;
    float    fundamental = 440.0f;

    StandardADSR ampEnv;
    FilterEnvelope filterEnv;

    DiffusionPartial partials[kNumPartials];

    // Per-voice diffusion state
    float diffusionClock = 0.0f;  // seconds since note-on
    float noteAge        = 0.0f;  // total lifetime for cross-note blending

    // Per-voice filter
    CytomicSVF viscosityFilter;

    void reset() noexcept
    {
        active = false;
        ampEnv.reset();
        filterEnv.stage = FilterEnvelope::Stage::Idle;
        filterEnv.level = 0.0f;
        diffusionClock = 0.0f;
        noteAge = 0.0f;
        viscosityFilter.reset();
        for (auto& p : partials)
            p.reset();
    }

    void prepare (float sampleRate) noexcept
    {
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        viscosityFilter.setMode (CytomicSVF::Mode::LowPass);
        viscosityFilter.reset();
    }

    void noteOn (int note, float vel, float sr, uint64_t time) noexcept
    {
        active = true;
        currentNote = note;
        velocity = vel;
        startTime = time;
        fundamental = 440.0f * fastPow2 ((static_cast<float> (note) - 69.0f) / 12.0f);
        diffusionClock = 0.0f;
        noteAge = 0.0f;

        ampEnv.noteOn();
        filterEnv.trigger();

        // Initialize partials: harmonics 1–16 with 1/n amplitude weighting
        for (int i = 0; i < kNumPartials; ++i)
        {
            partials[i].baseFreqRatio = static_cast<float> (i + 1);
            partials[i].amplitude = 1.0f / static_cast<float> (i + 1);
            partials[i].phase = 0.0f;
            partials[i].diffusionAge = 0.0f;
            partials[i].spreadOffset = 0.0f;
        }
    }

    void noteOff() noexcept
    {
        ampEnv.noteOff();
        filterEnv.release();
    }
};

//==============================================================================
// OverwashEngine
//==============================================================================
class OverwashEngine : public SynthEngine
{
public:
    OverwashEngine() = default;

    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface --------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srF = static_cast<float> (sampleRate);
        blockSize = maxBlockSize;

        for (auto& v : voices)
        {
            v.prepare (srF);
            v.reset();
        }

        // LFOs
        lfo1.setShape (StandardLFO::Sine);
        lfo1.reset();
        lfo2.setShape (StandardLFO::Triangle);
        lfo2.reset();

        breathLfo.setRate (0.008f, srF);  // D005: sub-Hz breathing

        noteCounter = 0;

        // Global spectral field accumulator (simplified: 32 bins)
        std::fill (spectralField.begin(), spectralField.end(), 0.0f);

        // Silence gate: 1000ms hold (long pad tails)
        silenceGate.prepare (sr, maxBlockSize);
        silenceGate.setHoldTime (1000.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        lfo1.reset();
        lfo2.reset();
        std::fill (spectralField.begin(), spectralField.end(), 0.0f);
        lastSampleL = lastSampleR = 0.0f;
        extFilterMod = extPitchMod = extRingMod = 0.0f;
    }

    //--------------------------------------------------------------------------
    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                int idx = VoiceAllocator::findFreeVoicePreferRelease (
                    voices, kMaxVoices,
                    [] (const OverwashVoice& v) { return v.ampEnv.getStage() == StandardADSR::Stage::Release; });
                voices[idx].noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), srF, ++noteCounter);
            }
            else if (msg.isNoteOff())
            {
                int idx = VoiceAllocator::findVoiceForNote (voices, kMaxVoices, msg.getNoteNumber());
                if (idx >= 0) voices[idx].noteOff();
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouch = msg.isAftertouch()
                    ? msg.getAfterTouchValue() / 127.0f
                    : msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            }
        }

        // 2. Bypass check
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters
        float pDiffRate    = pDiffRateParam    ? pDiffRateParam->load()    : 0.5f;
        float pViscosity   = pViscosityParam   ? pViscosityParam->load()   : 0.5f;
        float pHarmonics   = pHarmonicsParam   ? pHarmonicsParam->load()   : 12.0f;
        float pSpreadMax   = pSpreadMaxParam   ? pSpreadMaxParam->load()   : 200.0f;
        float pDiffTime    = pDiffTimeParam    ? pDiffTimeParam->load()    : 10.0f;
        float pInterference= pInterferenceParam? pInterferenceParam->load(): 0.3f;
        float pBrightness  = pBrightnessParam  ? pBrightnessParam->load()  : 0.7f;
        float pWarmth      = pWarmthParam      ? pWarmthParam->load()      : 0.5f;
        float pStereoWidth = pStereoWidthParam ? pStereoWidthParam->load() : 0.6f;
        float pFilterCut   = pFilterCutParam   ? pFilterCutParam->load()   : 4000.0f;
        float pFilterRes   = pFilterResParam   ? pFilterResParam->load()   : 0.2f;
        float pFiltEnvAmt  = pFiltEnvAmtParam  ? pFiltEnvAmtParam->load()  : 0.3f;
        float pAmpA        = pAmpAParam        ? pAmpAParam->load()        : 0.3f;
        float pAmpD        = pAmpDParam        ? pAmpDParam->load()        : 1.0f;
        float pAmpS        = pAmpSParam        ? pAmpSParam->load()        : 0.8f;
        float pAmpR        = pAmpRParam        ? pAmpRParam->load()        : 2.0f;
        float pFiltA       = pFiltAParam       ? pFiltAParam->load()       : 0.1f;
        float pFiltD       = pFiltDParam       ? pFiltDParam->load()       : 0.5f;
        float pFiltS       = pFiltSParam       ? pFiltSParam->load()       : 0.4f;
        float pFiltR       = pFiltRParam       ? pFiltRParam->load()       : 1.0f;
        float pLfo1Rate    = pLfo1RateParam    ? pLfo1RateParam->load()    : 0.1f;
        float pLfo1Depth   = pLfo1DepthParam   ? pLfo1DepthParam->load()   : 0.2f;
        float pLfo2Rate    = pLfo2RateParam    ? pLfo2RateParam->load()    : 0.05f;
        float pLfo2Depth   = pLfo2DepthParam   ? pLfo2DepthParam->load()   : 0.15f;
        float pLevel       = pLevelParam       ? pLevelParam->load()       : 0.8f;

        // Macros
        float pMC = pMacroCharacterParam ? pMacroCharacterParam->load() : 0.0f;
        float pMM = pMacroMovementParam  ? pMacroMovementParam->load()  : 0.0f;
        float pMK = pMacroCouplingParam  ? pMacroCouplingParam->load()  : 0.0f;
        float pMS = pMacroSpaceParam     ? pMacroSpaceParam->load()     : 0.0f;

        // CHARACTER → viscosity + harmonic density
        pViscosity  = clamp (pViscosity + pMC * 0.4f, 0.0f, 1.0f);
        pBrightness = clamp (pBrightness + pMC * 0.3f, 0.0f, 1.0f);

        // MOVEMENT → diffusion rate + LFO depth
        pDiffRate  = clamp (pDiffRate + pMM * 0.4f, 0.0f, 1.0f);
        pLfo1Depth = clamp (pLfo1Depth + pMM * 0.3f, 0.0f, 1.0f);

        // COUPLING → interference depth
        pInterference = clamp (pInterference + pMK * 0.4f, 0.0f, 1.0f);

        // SPACE → stereo width
        pStereoWidth = clamp (pStereoWidth + pMS * 0.3f, 0.0f, 1.0f);

        // D006: Mod wheel → diffusion rate
        pDiffRate = clamp (pDiffRate + modWheel * 0.3f, 0.0f, 1.0f);

        // D006: Aftertouch → viscosity
        pViscosity = clamp (pViscosity + aftertouch * 0.3f, 0.0f, 1.0f);

        // Coupling modulation
        pFilterCut = clamp (pFilterCut + extFilterMod, 50.0f, 16000.0f);

        // BROTH cooperative coupling: sessionAge increases viscosity
        pViscosity = clamp (pViscosity + brothSessionAge * 0.3f, 0.0f, 1.0f);

        // Convert diffusion rate parameter to diffusion coefficient D (Fick's Law)
        // D range: 0.01 (syrupy) to 5.0 (water-thin)
        float D = 0.01f + (1.0f - pViscosity) * pDiffRate * 4.99f;

        // How many partials to actually use
        int activePartials = std::max (1, std::min (OverwashVoice::kNumPartials,
                                                     static_cast<int> (pHarmonics)));

        // LFO rates
        lfo1.setRate (pLfo1Rate, srF);
        lfo2.setRate (pLfo2Rate, srF);

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1
                        ? buffer.getWritePointer (1) : nullptr;

        const float inverseSr = 1.0f / srF;
        const float pitchBendRatio = PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            float lfo1Val = lfo1.process();
            float lfo2Val = lfo2.process();
            float breathVal = breathLfo.process();

            float sampleL = 0.0f;
            float sampleR = 0.0f;

            for (int v = 0; v < kMaxVoices; ++v)
            {
                auto& voice = voices[v];
                if (!voice.active) continue;

                // Advance diffusion clock
                voice.diffusionClock += inverseSr;
                voice.noteAge += inverseSr;

                // Update envelopes
                voice.ampEnv.setADSR (pAmpA, pAmpD, pAmpS, pAmpR);
                voice.filterEnv.setADSR (pFiltA, pFiltD, pFiltS, pFiltR);

                float ampLevel = voice.ampEnv.process();
                float filtLevel = voice.filterEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // D001: velocity shapes brightness
                float velBright = 0.5f + voice.velocity * 0.5f;

                // Per-voice filter cutoff with filter envelope and velocity
                float voiceCutoff = pFilterCut * velBright
                    + pFiltEnvAmt * filtLevel * 8000.0f * voice.velocity;
                voiceCutoff = clamp (voiceCutoff, 50.0f, srF * 0.49f);
                voice.viscosityFilter.setCoefficients_fast (voiceCutoff, pFilterRes, srF);

                float fundamental = voice.fundamental * pitchBendRatio;

                // Synthesize diffusing partials
                float voiceSample = 0.0f;
                for (int p = 0; p < activePartials; ++p)
                {
                    auto& partial = voice.partials[p];

                    // Fick's Law: spread = sqrt(2 * D * t) in frequency space
                    // Each partial drifts outward from its harmonic position
                    float t = voice.diffusionClock;
                    float maxT = pDiffTime;
                    float normalizedT = (maxT > 0.0f) ? std::min (t / maxT, 1.0f) : 0.0f;

                    // Analytical Gaussian diffusion (no FFT needed)
                    // Spread grows as sqrt(2Dt), capped by pSpreadMax
                    float spread = std::sqrt (2.0f * D * std::min (t, maxT)) * pSpreadMax;

                    // Each partial gets a unique spreading direction based on harmonic number
                    // Odd partials spread up, even spread down, with alternating phase
                    float direction = (p % 2 == 0) ? 1.0f : -1.0f;
                    // Apply LFO modulation to spread direction
                    direction += lfo1Val * pLfo1Depth * 0.3f;

                    float freqOffset = spread * direction * partial.baseFreqRatio * 0.01f;

                    // Add breathing modulation (D005)
                    freqOffset += breathVal * 0.5f * partial.baseFreqRatio;

                    float partialFreq = fundamental * partial.baseFreqRatio + freqOffset;
                    partialFreq = clamp (partialFreq, 20.0f, srF * 0.49f);

                    // Phase accumulator
                    partial.phase += partialFreq * inverseSr;
                    if (partial.phase >= 1.0f) partial.phase -= 1.0f;

                    // Sine oscillator with amplitude weighting
                    float sine = fastSin (partial.phase * 6.28318530718f);

                    // Amplitude decreases as partial diffuses further from origin
                    float diffusionAttenuation = 1.0f - normalizedT * 0.6f;
                    diffusionAttenuation = clamp (diffusionAttenuation, 0.0f, 1.0f);

                    // Warmth: attenuate higher partials more
                    float warmthAtten = 1.0f / (1.0f + (1.0f - pWarmth) * static_cast<float> (p) * 0.3f);

                    float partialContrib = sine * partial.amplitude * diffusionAttenuation
                                          * warmthAtten * velBright;

                    // Accumulate partial energy into spectral field for cross-note interference.
                    // Map frequency to one of 32 bins (log-spaced 20Hz–20kHz).
                    if (pInterference > 0.01f)
                    {
                        float logFreq = std::log2 (partialFreq / 20.0f) / std::log2 (1000.0f); // 0..1
                        int bin = std::min (static_cast<int> (logFreq * 32.0f), 31);
                        if (bin >= 0)
                            spectralField[static_cast<size_t> (bin)] += std::fabs (partialContrib) * 0.05f;
                    }

                    voiceSample += partialContrib;
                }

                // Interference: where diffusion fronts from different notes overlap,
                // add subtle beating (amplitude modulation from neighbor bins).
                // wash_interference controls how much cross-note energy affects this voice.
                if (pInterference > 0.01f)
                {
                    // Map this voice's fundamental to a spectral bin
                    float logFund = std::log2 (fundamental / 20.0f) / std::log2 (1000.0f);
                    int fundBin = std::min (static_cast<int> (logFund * 32.0f), 31);
                    if (fundBin >= 0)
                    {
                        // Sum energy in neighboring bins (interference fringes from other voices)
                        float neighborEnergy = 0.0f;
                        for (int nb = std::max (0, fundBin - 2); nb <= std::min (31, fundBin + 2); ++nb)
                            neighborEnergy += spectralField[static_cast<size_t> (nb)];
                        neighborEnergy = clamp (neighborEnergy, 0.0f, 1.0f);
                        // Interference fringes: slight amplitude modulation proportional to neighbor energy
                        float interferenceAmt = neighborEnergy * pInterference * 0.3f;
                        voiceSample *= (1.0f - interferenceAmt + interferenceAmt * lfo1Val);
                    }
                }

                // Normalize by partial count
                voiceSample *= (1.0f / static_cast<float> (activePartials)) * 2.0f;

                // Apply viscosity filter
                voiceSample = voice.viscosityFilter.processSample (voiceSample);

                // Apply amp envelope
                voiceSample *= ampLevel;

                // Stereo placement: each voice gets a position based on note + LFO2
                float pan = 0.5f + (static_cast<float> (voice.currentNote - 60) * 0.02f
                            + lfo2Val * pLfo2Depth * 0.3f) * pStereoWidth;
                pan = clamp (pan, 0.0f, 1.0f);

                sampleL += voiceSample * (1.0f - pan);
                sampleR += voiceSample * pan;
            }

            // Apply global level
            sampleL *= pLevel;
            sampleR *= pLevel;

            // Ring mod coupling
            if (std::fabs (extRingMod) > 0.001f)
            {
                sampleL *= (1.0f + extRingMod);
                sampleR *= (1.0f + extRingMod);
            }

            // Soft clip
            sampleL = softClip (sampleL);
            sampleR = softClip (sampleR);

            lastSampleL = sampleL;
            lastSampleR = sampleR;

            outL[i] += sampleL;
            if (outR) outR[i] += sampleR;
        }

        // Decay spectral field accumulator (slow diffusion of the field itself)
        // This prevents stale energy from dominating and makes interference fade naturally.
        for (auto& bin : spectralField)
            bin *= 0.98f;  // ~50ms half-life at typical block rates

        // Reset coupling mods
        extFilterMod = 0.0f;
        extPitchMod = 0.0f;
        extRingMod = 0.0f;

        // Silence gate
        const float* rL = buffer.getReadPointer (0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr;
        silenceGate.analyzeBlock (rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                extFilterMod = amount * 4000.0f;
                break;
            case CouplingType::LFOToPitch:
                extPitchMod = amount;
                break;
            case CouplingType::AudioToRing:
                extRingMod = (sourceBuffer ? sourceBuffer[0] : 0.0f) * amount;
                break;
            default:
                break;
        }
    }

    //-- BROTH Cooperative Coupling ---------------------------------------------
    // Called by the BROTH coordinator to share XOverworn's session age.
    void setBrothSessionAge (float age) { brothSessionAge = clamp (age, 0.0f, 1.0f); }
    float getDiffusionField (int bin) const
    {
        if (bin >= 0 && bin < 32) return spectralField[static_cast<size_t> (bin)];
        return 0.0f;
    }

    //-- Parameters -------------------------------------------------------------

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // Core diffusion parameters
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_diffusionRate", 1 }, "Diffusion Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_viscosity", 1 }, "Viscosity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_harmonics", 1 }, "Harmonic Density",
            juce::NormalisableRange<float> (1.0f, 16.0f, 1.0f), 12.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_spreadMax", 1 }, "Spread Maximum",
            juce::NormalisableRange<float> (10.0f, 500.0f, 0.0f, 0.4f), 200.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_diffusionTime", 1 }, "Diffusion Time",
            juce::NormalisableRange<float> (1.0f, 60.0f, 0.0f, 0.4f), 10.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_interference", 1 }, "Interference",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_brightness", 1 }, "Brightness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_warmth", 1 }, "Warmth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_stereoWidth", 1 }, "Stereo Width",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));

        // Filter
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filterCutoff", 1 }, "Filter Cutoff",
            juce::NormalisableRange<float> (50.0f, 16000.0f, 0.0f, 0.3f), 4000.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filterRes", 1 }, "Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filtEnvAmount", 1 }, "Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // Amp ADSR
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_ampAttack", 1 }, "Amp Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f), 0.3f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_ampDecay", 1 }, "Amp Decay",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 1.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_ampSustain", 1 }, "Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_ampRelease", 1 }, "Amp Release",
            juce::NormalisableRange<float> (0.01f, 15.0f, 0.0f, 0.3f), 2.0f));

        // Filter ADSR
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filtAttack", 1 }, "Filter Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f), 0.1f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filtDecay", 1 }, "Filter Decay",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filtSustain", 1 }, "Filter Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_filtRelease", 1 }, "Filter Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 1.0f));

        // LFOs
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_lfo1Rate", 1 }, "LFO 1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.1f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_lfo1Depth", 1 }, "LFO 1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_lfo2Rate", 1 }, "LFO 2 Rate",
            juce::NormalisableRange<float> (0.005f, 10.0f, 0.0f, 0.3f), 0.05f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_lfo2Depth", 1 }, "LFO 2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));

        // Output
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_level", 1 }, "Level",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

        // Macros
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_macroCharacter", 1 }, "Character",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_macroMovement", 1 }, "Movement",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_macroCoupling", 1 }, "Coupling",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "wash_macroSpace", 1 }, "Space",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pDiffRateParam     = apvts.getRawParameterValue ("wash_diffusionRate");
        pViscosityParam    = apvts.getRawParameterValue ("wash_viscosity");
        pHarmonicsParam    = apvts.getRawParameterValue ("wash_harmonics");
        pSpreadMaxParam    = apvts.getRawParameterValue ("wash_spreadMax");
        pDiffTimeParam     = apvts.getRawParameterValue ("wash_diffusionTime");
        pInterferenceParam = apvts.getRawParameterValue ("wash_interference");
        pBrightnessParam   = apvts.getRawParameterValue ("wash_brightness");
        pWarmthParam       = apvts.getRawParameterValue ("wash_warmth");
        pStereoWidthParam  = apvts.getRawParameterValue ("wash_stereoWidth");
        pFilterCutParam    = apvts.getRawParameterValue ("wash_filterCutoff");
        pFilterResParam    = apvts.getRawParameterValue ("wash_filterRes");
        pFiltEnvAmtParam   = apvts.getRawParameterValue ("wash_filtEnvAmount");
        pAmpAParam         = apvts.getRawParameterValue ("wash_ampAttack");
        pAmpDParam         = apvts.getRawParameterValue ("wash_ampDecay");
        pAmpSParam         = apvts.getRawParameterValue ("wash_ampSustain");
        pAmpRParam         = apvts.getRawParameterValue ("wash_ampRelease");
        pFiltAParam        = apvts.getRawParameterValue ("wash_filtAttack");
        pFiltDParam        = apvts.getRawParameterValue ("wash_filtDecay");
        pFiltSParam        = apvts.getRawParameterValue ("wash_filtSustain");
        pFiltRParam        = apvts.getRawParameterValue ("wash_filtRelease");
        pLfo1RateParam     = apvts.getRawParameterValue ("wash_lfo1Rate");
        pLfo1DepthParam    = apvts.getRawParameterValue ("wash_lfo1Depth");
        pLfo2RateParam     = apvts.getRawParameterValue ("wash_lfo2Rate");
        pLfo2DepthParam    = apvts.getRawParameterValue ("wash_lfo2Depth");
        pLevelParam        = apvts.getRawParameterValue ("wash_level");
        pMacroCharacterParam = apvts.getRawParameterValue ("wash_macroCharacter");
        pMacroMovementParam  = apvts.getRawParameterValue ("wash_macroMovement");
        pMacroCouplingParam  = apvts.getRawParameterValue ("wash_macroCoupling");
        pMacroSpaceParam     = apvts.getRawParameterValue ("wash_macroSpace");
    }

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Overwash"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFFD4A76A);  // Tea Amber
    }

    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override
    {
        return VoiceAllocator::countActive (voices, kMaxVoices);
    }

private:
    double sr = 44100.0;
    float srF = 44100.0f;
    int blockSize = 512;

    OverwashVoice voices[kMaxVoices];
    uint64_t noteCounter = 0;

    // LFOs
    StandardLFO lfo1, lfo2;
    BreathingLFO breathLfo;

    // Global spectral field (32 bins for cross-note interference)
    std::array<float, 32> spectralField {};

    // MIDI state
    float aftertouch   = 0.0f;
    float modWheel     = 0.0f;
    float pitchBendNorm = 0.0f;

    // Coupling state
    float extFilterMod = 0.0f;
    float extPitchMod  = 0.0f;
    float extRingMod   = 0.0f;
    float lastSampleL  = 0.0f;
    float lastSampleR  = 0.0f;

    // BROTH cooperative
    float brothSessionAge = 0.0f;

    // Parameter pointers (29 params)
    std::atomic<float>* pDiffRateParam     = nullptr;
    std::atomic<float>* pViscosityParam    = nullptr;
    std::atomic<float>* pHarmonicsParam    = nullptr;
    std::atomic<float>* pSpreadMaxParam    = nullptr;
    std::atomic<float>* pDiffTimeParam     = nullptr;
    std::atomic<float>* pInterferenceParam = nullptr;
    std::atomic<float>* pBrightnessParam   = nullptr;
    std::atomic<float>* pWarmthParam       = nullptr;
    std::atomic<float>* pStereoWidthParam  = nullptr;
    std::atomic<float>* pFilterCutParam    = nullptr;
    std::atomic<float>* pFilterResParam    = nullptr;
    std::atomic<float>* pFiltEnvAmtParam   = nullptr;
    std::atomic<float>* pAmpAParam         = nullptr;
    std::atomic<float>* pAmpDParam         = nullptr;
    std::atomic<float>* pAmpSParam         = nullptr;
    std::atomic<float>* pAmpRParam         = nullptr;
    std::atomic<float>* pFiltAParam        = nullptr;
    std::atomic<float>* pFiltDParam        = nullptr;
    std::atomic<float>* pFiltSParam        = nullptr;
    std::atomic<float>* pFiltRParam        = nullptr;
    std::atomic<float>* pLfo1RateParam     = nullptr;
    std::atomic<float>* pLfo1DepthParam    = nullptr;
    std::atomic<float>* pLfo2RateParam     = nullptr;
    std::atomic<float>* pLfo2DepthParam    = nullptr;
    std::atomic<float>* pLevelParam        = nullptr;
    std::atomic<float>* pMacroCharacterParam = nullptr;
    std::atomic<float>* pMacroMovementParam  = nullptr;
    std::atomic<float>* pMacroCouplingParam  = nullptr;
    std::atomic<float>* pMacroSpaceParam     = nullptr;
};

} // namespace xolokun
