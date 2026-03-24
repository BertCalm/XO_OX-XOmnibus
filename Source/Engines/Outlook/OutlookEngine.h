#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OutlookEngine — "Panoramic Visionary" synth engine.
//
// XOutlook is a pad/lead synthesizer that sees across the horizon. It combines
// panoramic wavetable scanning with a parallax stereo field that shifts
// perspective as you play — near objects (low notes) move slowly, distant
// objects (high notes) shimmer and drift. The result: expansive, luminous
// soundscapes that feel three-dimensional.
//
// DSP Architecture:
//   1. PANORAMA OSCILLATOR: Dual wavetable with horizon scanning — a single
//      macro sweeps both tables in opposite directions, creating evolving
//      interference patterns. 8 built-in wave shapes per table.
//
//   2. PARALLAX STEREO FIELD: Note pitch controls stereo depth. Low notes
//      produce narrow, grounded images. High notes spread wide with
//      micro-detuned parallax layers. Creates natural depth perspective.
//
//   3. VISTA FILTER: Dual SVF (LP + HP in series) with a "horizon line"
//      parameter — sweeping it opens or closes the spectral vista like
//      adjusting a camera's aperture. Velocity-scaled (D001).
//
//   4. AURORA MOD: Two LFOs feed a luminosity modulator — when both LFOs
//      align (conjunction), brightness peaks. When opposed, the sound
//      darkens. Creates organic breathing that follows celestial logic.
//
// Accent: Horizon Indigo #4169E1
// Creature: The Albatross — Surface Soarer
// feliX/Oscar polarity: Oscar-dominant (0.25/0.75)
//
// Designed for: Atmosphere, Aether, Prism moods
// Best coupling partners: OPENSKY (shimmer stacking), OMBRE (memory/vision),
//   OPAL (granular parallax), OXBOW (entangled reverb tail)
//==============================================================================
class OutlookEngine : public SynthEngine
{
public:
    OutlookEngine() = default;

    //-- SynthEngine identity ---------------------------------------------------

    juce::String getEngineId() const override { return "Outlook"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF4169E1); }
    int getMaxVoices() const override { return 8; }

    //-- Lifecycle --------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        prepareSilenceGate (sampleRate, maxBlockSize, 500.0f); // Long pad tails

        couplingBuffer.setSize (2, maxBlockSize);
        couplingBuffer.clear();

        // Prepare per-voice state
        for (auto& v : voices)
        {
            v.active = false;
            v.phase1 = 0.0;
            v.phase2 = 0.0;
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.filterLP.reset();
            v.filterLP.setMode (CytomicSVF::Mode::LowPass);
            v.filterHP.reset();
            v.filterHP.setMode (CytomicSVF::Mode::HighPass);
        }

        // LFOs (D005: rate floor 0.01 Hz)
        lfo1.reset();
        lfo2.reset();

        // FX buffers
        delayBufL.fill (0.0f);
        delayBufR.fill (0.0f);
        delayWritePos = 0;
        reverbBuf.fill (0.0f);
        reverbWritePos = 0;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.ampEnv.reset();
            v.filterEnv.reset();
            v.filterLP.reset();
            v.filterHP.reset();
            v.phase1 = 0.0;
            v.phase2 = 0.0;
        }
        lfo1.reset();
        lfo2.reset();
    }

    //-- Audio rendering --------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // ---- MIDI parsing ----
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();

            if (msg.isNoteOn())
            {
                wakeSilenceGate();
                allocateVoice (msg.getNoteNumber(), msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                releaseVoice (msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel = msg.getControllerValue() / 127.0f; // D006
            }
            else if (msg.isChannelPressure())
            {
                aftertouch = msg.getChannelPressureValue() / 127.0f; // D006
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            }
        }

        // ---- Silence gate bypass ----
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ---- Snapshot parameters ----
        const float pHorizon      = pHorizonScan  ? pHorizonScan->load()  : 0.5f;
        const float pParallax     = pParallaxAmt   ? pParallaxAmt->load()   : 0.5f;
        const float pVistaOpen    = pVistaLine     ? pVistaLine->load()     : 0.7f;
        const float pFilterRes    = pResonance     ? pResonance->load()     : 0.3f;
        const float pAtt          = pAttack        ? pAttack->load()        : 0.1f;
        const float pDec          = pDecay         ? pDecay->load()         : 0.3f;
        const float pSus          = pSustain       ? pSustain->load()       : 0.7f;
        const float pRel          = pRelease       ? pRelease->load()       : 0.8f;
        const float pFiltEnvAmt   = pFilterEnvAmt  ? pFilterEnvAmt->load()  : 0.5f;
        const float pLfo1Rate     = pLfo1RateParam ? pLfo1RateParam->load() : 0.5f;
        const float pLfo1Dep      = pLfo1DepParam  ? pLfo1DepParam->load()  : 0.3f;
        const float pLfo2Rate     = pLfo2RateParam ? pLfo2RateParam->load() : 0.3f;
        const float pLfo2Dep      = pLfo2DepParam  ? pLfo2DepParam->load()  : 0.2f;
        const float pWave1        = pWaveShape1    ? pWaveShape1->load()    : 0.0f;
        const float pWave2        = pWaveShape2    ? pWaveShape2->load()    : 2.0f;
        const float pOscMix       = pOscMixParam   ? pOscMixParam->load()   : 0.5f;
        const float pReverb       = pReverbMix     ? pReverbMix->load()     : 0.3f;
        const float pDelay        = pDelayMix      ? pDelayMix->load()      : 0.0f;
        const float pMacroChar    = pMacroCharacter? pMacroCharacter->load(): 0.5f;
        const float pMacroMov     = pMacroMovement ? pMacroMovement->load() : 0.0f;
        const float pMacroCouple  = pMacroCoupling ? pMacroCoupling->load() : 0.0f;
        const float pMacroSpc     = pMacroSpace    ? pMacroSpace->load()    : 0.3f;
        const float pModWheelDep  = pModWheelDepth ? pModWheelDepth->load() : 0.5f;
        const float pAfterDep     = pAftertouchDep ? pAftertouchDep->load() : 0.5f;

        // ---- LFO tick (block-rate) ----
        lfo1.setRate (pLfo1Rate, static_cast<float> (sr));
        lfo2.setRate (pLfo2Rate, static_cast<float> (sr));

        // ---- Render per-sample ----
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        for (int s = 0; s < numSamples; ++s)
        {
            const float lfo1Val = lfo1.process();
            const float lfo2Val = lfo2.process();

            // Aurora mod: conjunction brightness
            const float conjunction = (lfo1Val + lfo2Val) * 0.5f;
            const float auroraLuminosity = 0.5f + conjunction * 0.5f;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& v : voices)
            {
                if (!v.active && v.ampEnv.getState() == EnvState::Idle)
                    continue;

                ++v.age; // Track voice age for oldest-voice stealing

                // Amp envelope
                const float ampEnvVal = v.ampEnv.tick (pAtt, pDec, pSus, pRel, sr);
                if (ampEnvVal < 1e-7f && v.ampEnv.getState() == EnvState::Idle)
                {
                    v.active = false;
                    continue;
                }

                // Filter envelope
                const float filtEnvVal = v.filterEnv.tick (pAtt * 0.5f, pDec * 0.8f, 0.0f, pRel * 0.5f, sr);

                // Velocity → timbre (D001): velocity scales filter brightness
                const float velFilterMod = v.velocity * 0.6f + 0.4f;

                // Pitch with bend
                const float freq = v.baseFreq * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);
                const double phaseInc = freq / sr;

                // Macro CHARACTER + coupling modulates horizon scan position
                const float cplScaleH = 1.0f + pMacroCouple * 3.0f;
                const float horizonPos = std::clamp (pHorizon + pMacroChar * 0.5f - 0.25f + couplingHorizonMod * cplScaleH, 0.0f, 1.0f);

                // Panorama oscillator: dual wavetable scanning in opposite directions
                const float scan1 = horizonPos;
                const float scan2 = 1.0f - horizonPos;

                const float osc1 = renderWave (v.phase1, static_cast<int> (pWave1), scan1);
                const float osc2 = renderWave (v.phase2, static_cast<int> (pWave2), scan2);

                v.phase1 += phaseInc;
                v.phase2 += phaseInc * 1.001; // Slight detune for width
                if (v.phase1 >= 1.0) v.phase1 -= 1.0;
                if (v.phase2 >= 1.0) v.phase2 -= 1.0;

                const float oscMix = osc1 * (1.0f - pOscMix) + osc2 * pOscMix;

                // Vista filter: horizon line opens/closes spectral view
                // COUPLING macro scales all coupling input sensitivity
                const float cplScale = 1.0f + pMacroCouple * 3.0f; // 0→1x, 1→4x
                const float baseCutoff = 200.0f + pVistaOpen * 18000.0f + couplingFilterMod * cplScale;
                const float envCutoff = baseCutoff * (1.0f + filtEnvVal * pFiltEnvAmt * velFilterMod);
                // MOVEMENT macro directly scales LFO→filter depth (no triple attenuation)
                const float movementAmt = pMacroMov + modWheel * pModWheelDep;
                const float modCutoff = envCutoff * (1.0f + lfo1Val * pLfo1Dep * std::max (movementAmt, 0.1f));
                const float finalCutoff = std::clamp (modCutoff + aftertouch * pAfterDep * 4000.0f, 20.0f, 20000.0f);

                v.filterLP.setCoefficients_fast (finalCutoff, pFilterRes, static_cast<float> (sr));
                const float filtered = v.filterLP.processSample (oscMix);

                // HP for clearing low mud based on horizon
                const float hpCutoff = 20.0f + (1.0f - pVistaOpen) * 300.0f;
                v.filterHP.setCoefficients_fast (hpCutoff, 0.5f, static_cast<float> (sr));
                const float cleaned = v.filterHP.processSample (filtered);

                // Aurora luminosity modulates amplitude
                const float luminosity = 1.0f + (auroraLuminosity - 0.5f) * pLfo2Dep * 0.4f;
                const float monoOut = cleaned * ampEnvVal * luminosity * v.velocity;

                // Parallax stereo: high notes spread wider (+ coupling modulation)
                const float noteNorm = (v.note - 36.0f) / 60.0f; // 0=C2, 1=C7
                const float cplScaleP = 1.0f + pMacroCouple * 3.0f;
                const float parallaxTotal = std::clamp (pParallax + couplingParallaxMod * cplScaleP, 0.0f, 1.0f);
                const float spread = std::clamp (noteNorm * parallaxTotal, 0.0f, 1.0f);
                // Equal-power complementary panning
                const float panAngle = spread * 0.4f * (1.0f + lfo2Val * 0.1f);
                const float gainL = std::cos (panAngle * juce::MathConstants<float>::halfPi);
                const float gainR = std::sin (panAngle * juce::MathConstants<float>::halfPi + juce::MathConstants<float>::halfPi * 0.5f);

                mixL += monoOut * gainL;
                mixR += monoOut * gainR;
            }

            // Macro SPACE → reverb/delay depth
            const float spaceAmt = std::clamp (pMacroSpc + pReverb, 0.0f, 1.0f);
            const float delayAmt = std::clamp (pDelay + pMacroCouple * 0.3f, 0.0f, 1.0f);

            // Ping-pong delay (0.375s L, 0.25s R at 48kHz)
            const int delayWriteIdx = delayWritePos % kDelayBufSize;
            const int delayReadL = (delayWritePos - static_cast<int> (0.375 * sr) + kDelayBufSize * 4) % kDelayBufSize;
            const int delayReadR = (delayWritePos - static_cast<int> (0.25 * sr) + kDelayBufSize * 4) % kDelayBufSize;
            const float delayOutL = FastMath::flushDenormal (delayBufL[static_cast<size_t> (delayReadL)]);
            const float delayOutR = FastMath::flushDenormal (delayBufR[static_cast<size_t> (delayReadR)]);
            delayBufL[static_cast<size_t> (delayWriteIdx)] = mixR * 0.5f + delayOutR * 0.35f; // Cross-feed for ping-pong
            delayBufR[static_cast<size_t> (delayWriteIdx)] = mixL * 0.5f + delayOutL * 0.35f;
            delayWritePos = (delayWritePos + 1) % kDelayBufSize;

            // Diffusion reverb (4-tap allpass with feedback)
            const int revIdx = reverbWritePos % kReverbBufSize;
            const float revIn = (mixL + mixR) * 0.5f;
            float revOut = 0.0f;
            for (int t = 0; t < 4; ++t)
            {
                static constexpr int tapOffsets[4] = { 1117, 1543, 2371, 3079 };
                const int readIdx = (reverbWritePos - tapOffsets[t] + kReverbBufSize * 4) % kReverbBufSize;
                revOut += reverbBuf[static_cast<size_t> (readIdx)] * 0.25f;
            }
            reverbBuf[static_cast<size_t> (revIdx)] = FastMath::flushDenormal (revIn + revOut * 0.45f);
            reverbWritePos = (reverbWritePos + 1) % kReverbBufSize;

            outL[s] = mixL + revOut * spaceAmt + delayOutL * delayAmt;
            outR[s] = mixR + revOut * spaceAmt + delayOutR * delayAmt;

            // Denormal protection
            outL[s] = FastMath::flushDenormal (outL[s]);
            outR[s] = FastMath::flushDenormal (outR[s]);
        }

        // Coupling buffer
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy (
                couplingBuffer.getWritePointer (ch),
                buffer.getReadPointer (ch), numSamples);

        analyzeForSilenceGate (buffer, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (channel < couplingBuffer.getNumChannels() &&
            sampleIndex < couplingBuffer.getNumSamples())
            return couplingBuffer.getSample (channel, sampleIndex);
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        if (amount < 0.001f || sourceBuffer == nullptr) return;

        switch (type)
        {
            case CouplingType::AmpToFilter:
            {
                // Source amplitude modulates vista filter cutoff
                float rms = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    rms += sourceBuffer[i] * sourceBuffer[i];
                rms = std::sqrt (rms / static_cast<float> (numSamples));
                couplingFilterMod = rms * amount * 4000.0f;
                break;
            }
            case CouplingType::LFOToPitch:
            {
                // Source LFO modulates parallax depth
                float avg = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    avg += sourceBuffer[i];
                avg /= static_cast<float> (numSamples);
                couplingParallaxMod = avg * amount;
                break;
            }
            case CouplingType::EnvToMorph:
            {
                // Source envelope modulates horizon scan
                float peak = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    peak = std::max (peak, std::abs (sourceBuffer[i]));
                couplingHorizonMod = peak * amount;
                break;
            }
            default: break;
        }
    }

    //-- Parameters -------------------------------------------------------------

    juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        auto floatParam = [&] (const juce::String& id, const juce::String& name,
                               float min, float max, float def, float skew = 1.0f)
        {
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { id, 1 }, name,
                juce::NormalisableRange<float> (min, max, 0.0f, skew), def));
        };

        auto choiceParam = [&] (const juce::String& id, const juce::String& name,
                                juce::StringArray choices, int def)
        {
            params.push_back (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { id, 1 }, name, choices, def));
        };

        // Oscillator
        choiceParam ("look_waveShape1", "Wave 1", { "Sine", "Triangle", "Saw", "Square", "Pulse", "Super", "Noise", "Formant" }, 0);
        choiceParam ("look_waveShape2", "Wave 2", { "Sine", "Triangle", "Saw", "Square", "Pulse", "Super", "Noise", "Formant" }, 2);
        floatParam ("look_oscMix", "Osc Mix", 0.0f, 1.0f, 0.5f);
        floatParam ("look_horizonScan", "Horizon Scan", 0.0f, 1.0f, 0.5f);

        // Parallax
        floatParam ("look_parallaxAmount", "Parallax", 0.0f, 1.0f, 0.5f);

        // Vista filter
        floatParam ("look_vistaLine", "Vista Line", 0.0f, 1.0f, 0.7f);
        floatParam ("look_resonance", "Resonance", 0.0f, 1.0f, 0.3f);
        floatParam ("look_filterEnvAmt", "Filter Env Amt", 0.0f, 1.0f, 0.5f);

        // Envelope
        floatParam ("look_attack",  "Attack",  0.001f, 5.0f, 0.1f,  0.4f);
        floatParam ("look_decay",   "Decay",   0.001f, 5.0f, 0.3f,  0.4f);
        floatParam ("look_sustain", "Sustain", 0.0f,   1.0f, 0.7f);
        floatParam ("look_release", "Release", 0.001f, 10.0f, 0.8f, 0.4f);

        // LFOs (D005: rate floor 0.01 Hz)
        floatParam ("look_lfo1Rate",  "LFO 1 Rate",  0.01f, 20.0f, 0.5f, 0.3f);
        floatParam ("look_lfo1Depth", "LFO 1 Depth", 0.0f,  1.0f,  0.3f);
        floatParam ("look_lfo2Rate",  "LFO 2 Rate",  0.01f, 20.0f, 0.3f, 0.3f);
        floatParam ("look_lfo2Depth", "LFO 2 Depth", 0.0f,  1.0f,  0.2f);

        // Expression (D006)
        floatParam ("look_modWheelDepth",  "Mod Wheel Depth", 0.0f, 1.0f, 0.5f);
        floatParam ("look_aftertouchDepth", "Aftertouch Depth", 0.0f, 1.0f, 0.5f);

        // FX
        floatParam ("look_reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.3f);
        floatParam ("look_delayMix",  "Delay Mix",  0.0f, 1.0f, 0.0f);

        // Macros
        floatParam ("look_macroCharacter", "Character",  0.0f, 1.0f, 0.5f);
        floatParam ("look_macroMovement",  "Movement",   0.0f, 1.0f, 0.0f);
        floatParam ("look_macroCoupling",  "Coupling",   0.0f, 1.0f, 0.0f);
        floatParam ("look_macroSpace",     "Space",      0.0f, 1.0f, 0.3f);

        return { params.begin(), params.end() };
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pHorizonScan    = apvts.getRawParameterValue ("look_horizonScan");
        pParallaxAmt    = apvts.getRawParameterValue ("look_parallaxAmount");
        pVistaLine      = apvts.getRawParameterValue ("look_vistaLine");
        pResonance      = apvts.getRawParameterValue ("look_resonance");
        pFilterEnvAmt   = apvts.getRawParameterValue ("look_filterEnvAmt");
        pAttack         = apvts.getRawParameterValue ("look_attack");
        pDecay          = apvts.getRawParameterValue ("look_decay");
        pSustain        = apvts.getRawParameterValue ("look_sustain");
        pRelease        = apvts.getRawParameterValue ("look_release");
        pLfo1RateParam  = apvts.getRawParameterValue ("look_lfo1Rate");
        pLfo1DepParam   = apvts.getRawParameterValue ("look_lfo1Depth");
        pLfo2RateParam  = apvts.getRawParameterValue ("look_lfo2Rate");
        pLfo2DepParam   = apvts.getRawParameterValue ("look_lfo2Depth");
        pWaveShape1     = apvts.getRawParameterValue ("look_waveShape1");
        pWaveShape2     = apvts.getRawParameterValue ("look_waveShape2");
        pOscMixParam    = apvts.getRawParameterValue ("look_oscMix");
        pReverbMix      = apvts.getRawParameterValue ("look_reverbMix");
        pDelayMix       = apvts.getRawParameterValue ("look_delayMix");
        pMacroCharacter = apvts.getRawParameterValue ("look_macroCharacter");
        pMacroMovement  = apvts.getRawParameterValue ("look_macroMovement");
        pMacroCoupling  = apvts.getRawParameterValue ("look_macroCoupling");
        pMacroSpace     = apvts.getRawParameterValue ("look_macroSpace");
        pModWheelDepth  = apvts.getRawParameterValue ("look_modWheelDepth");
        pAftertouchDep  = apvts.getRawParameterValue ("look_aftertouchDepth");
    }

private:
    //-- Voice ------------------------------------------------------------------

    enum class EnvState { Idle, Attack, Decay, Sustain, Release };

    struct SimpleEnvelope
    {
        EnvState state = EnvState::Idle;
        float value = 0.0f;

        EnvState getState() const { return state; }

        void reset()    { state = EnvState::Idle; value = 0.0f; }
        void gate (bool on)
        {
            if (on)  state = EnvState::Attack;
            else if (state != EnvState::Idle) state = EnvState::Release;
        }

        float tick (float att, float dec, float sus, float rel, double sr)
        {
            const float minTime = 0.001f;
            switch (state)
            {
                case EnvState::Attack:
                    value += 1.0f / (std::max (att, minTime) * static_cast<float> (sr));
                    if (value >= 1.0f) { value = 1.0f; state = EnvState::Decay; }
                    break;
                case EnvState::Decay:
                    value -= (1.0f - sus) / (std::max (dec, minTime) * static_cast<float> (sr));
                    if (value <= sus) { value = sus; state = EnvState::Sustain; }
                    break;
                case EnvState::Sustain:
                    value = sus;
                    break;
                case EnvState::Release:
                    value -= value / (std::max (rel, minTime) * static_cast<float> (sr));
                    if (value < 1e-7f) { value = 0.0f; state = EnvState::Idle; }
                    break;
                case EnvState::Idle:
                    break;
            }
            return value;
        }
    };

    struct Voice
    {
        bool active = false;
        int note = 60;
        float velocity = 0.0f;
        float baseFreq = 440.0f;
        double phase1 = 0.0, phase2 = 0.0;
        uint32_t age = 0; // For oldest-voice stealing
        SimpleEnvelope ampEnv, filterEnv;
        CytomicSVF filterLP, filterHP;
    };

    static constexpr int kMaxVoices = 8;
    std::array<Voice, kMaxVoices> voices {};

    //-- Voice management -------------------------------------------------------

    void allocateVoice (int noteNumber, float vel)
    {
        // Find free voice or steal oldest
        int idx = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[static_cast<size_t> (i)].active &&
                voices[static_cast<size_t> (i)].ampEnv.getState() == EnvState::Idle)
            {
                idx = i;
                break;
            }
        }
        if (idx == -1)
        {
            // Steal oldest voice
            uint32_t maxAge = 0;
            idx = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[static_cast<size_t> (i)].age > maxAge)
                {
                    maxAge = voices[static_cast<size_t> (i)].age;
                    idx = i;
                }
            }
        }

        auto& v = voices[static_cast<size_t> (idx)];
        v.active = true;
        v.note = noteNumber;
        v.velocity = vel;
        v.baseFreq = 440.0f * std::pow (2.0f, (noteNumber - 69.0f) / 12.0f);
        v.phase1 = 0.0;
        v.phase2 = 0.0;
        v.age = 0;
        v.ampEnv.gate (true);
        v.filterEnv.gate (true);
        ++voiceCounter;
    }

    void releaseVoice (int noteNumber)
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == noteNumber)
            {
                v.ampEnv.gate (false);
                v.filterEnv.gate (false);
                v.active = false;
                break;
            }
        }
    }

    //-- Wavetable rendering ----------------------------------------------------

    static float renderWave (double phase, int shape, float scan)
    {
        const float p = static_cast<float> (phase);
        const float s = std::clamp (scan, 0.0f, 1.0f);

        // Morph between clean and harmonically rich versions using scan
        float base = 0.0f;
        switch (shape)
        {
            case 0: // Sine → scan adds odd harmonics (sine→square continuum)
            {
                const float sine = std::sin (p * juce::MathConstants<float>::twoPi);
                const float h3 = std::sin (p * juce::MathConstants<float>::twoPi * 3.0f) * 0.333f;
                const float h5 = std::sin (p * juce::MathConstants<float>::twoPi * 5.0f) * 0.2f;
                base = sine + s * (h3 + h5);
                base /= (1.0f + s * 0.533f); // Normalize
                break;
            }
            case 1: // Triangle → scan morphs toward saw
            {
                const float tri = 4.0f * std::abs (p - 0.5f) - 1.0f;
                const float saw = 2.0f * p - 1.0f;
                base = tri * (1.0f - s) + saw * s;
                break;
            }
            case 2: // Saw → scan adds 2nd harmonic (brightness)
            {
                const float saw = 2.0f * p - 1.0f;
                const float h2 = std::sin (p * juce::MathConstants<float>::twoPi * 2.0f) * 0.5f;
                base = saw + s * h2;
                base /= (1.0f + s * 0.5f);
                break;
            }
            case 3: // Square → scan morphs pulse width (50%→10%)
            {
                const float pw = 0.5f - s * 0.4f;
                base = p < pw ? 1.0f : -1.0f;
                break;
            }
            case 4: // Pulse (variable width via scan)
            {
                float pw = 0.1f + s * 0.8f;
                base = p < pw ? 1.0f : -1.0f;
                break;
            }
            case 5: // Super (detuned saw stack)
            {
                float sum = 0.0f;
                for (int d = 0; d < 3; ++d)
                {
                    float detune = (d - 1) * 0.005f * (1.0f + s);
                    double dp = phase + detune;
                    dp -= std::floor (dp);
                    sum += 2.0f * static_cast<float> (dp) - 1.0f;
                }
                base = sum / 3.0f;
                break;
            }
            case 6: // Noise (lock-free xorshift32 PRNG)
            {
                static thread_local uint32_t rng = 2463534242u;
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                base = static_cast<float> (rng) / static_cast<float> (0xFFFFFFFFu) * 2.0f - 1.0f;
                break;
            }
            case 7: // Formant-ish
            {
                float formantFreq = 1.0f + s * 4.0f;
                base = std::sin (p * juce::MathConstants<float>::twoPi)
                     * std::sin (p * juce::MathConstants<float>::twoPi * formantFreq);
                break;
            }
            default:
                base = std::sin (p * juce::MathConstants<float>::twoPi);
                break;
        }

        return base;
    }

    //-- Utility ----------------------------------------------------------------

    static float midiToFreq (float note)
    {
        return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
    }

    //-- State ------------------------------------------------------------------

    double sr = 44100.0;
    int blockSize = 512;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBendNorm = 0.0f;
    uint32_t voiceCounter = 0;

    // Delay buffers (ping-pong, ~0.5s at 48kHz)
    static constexpr int kDelayBufSize = 24000;
    std::array<float, kDelayBufSize> delayBufL {};
    std::array<float, kDelayBufSize> delayBufR {};
    int delayWritePos = 0;

    // Reverb buffer (4-tap allpass diffusion)
    static constexpr int kReverbBufSize = 4096;
    std::array<float, kReverbBufSize> reverbBuf {};
    int reverbWritePos = 0;

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingParallaxMod = 0.0f;
    float couplingHorizonMod = 0.0f;

    juce::AudioBuffer<float> couplingBuffer;

    StandardLFO lfo1, lfo2;

    // Cached parameter pointers
    std::atomic<float>* pHorizonScan    = nullptr;
    std::atomic<float>* pParallaxAmt    = nullptr;
    std::atomic<float>* pVistaLine      = nullptr;
    std::atomic<float>* pResonance      = nullptr;
    std::atomic<float>* pFilterEnvAmt   = nullptr;
    std::atomic<float>* pAttack         = nullptr;
    std::atomic<float>* pDecay          = nullptr;
    std::atomic<float>* pSustain        = nullptr;
    std::atomic<float>* pRelease        = nullptr;
    std::atomic<float>* pLfo1RateParam  = nullptr;
    std::atomic<float>* pLfo1DepParam   = nullptr;
    std::atomic<float>* pLfo2RateParam  = nullptr;
    std::atomic<float>* pLfo2DepParam   = nullptr;
    std::atomic<float>* pWaveShape1     = nullptr;
    std::atomic<float>* pWaveShape2     = nullptr;
    std::atomic<float>* pOscMixParam    = nullptr;
    std::atomic<float>* pReverbMix      = nullptr;
    std::atomic<float>* pDelayMix       = nullptr;
    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement  = nullptr;
    std::atomic<float>* pMacroCoupling  = nullptr;
    std::atomic<float>* pMacroSpace     = nullptr;
    std::atomic<float>* pModWheelDepth  = nullptr;
    std::atomic<float>* pAftertouchDep  = nullptr;
};

} // namespace xomnibus
