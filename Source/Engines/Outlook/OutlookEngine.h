// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

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
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF4169E1); }
    int getMaxVoices() const override { return 8; }

    //-- Lifecycle --------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f); // Long pad tails

        couplingBuffer.setSize(2, maxBlockSize);
        couplingBuffer.clear();

        // Prepare per-voice state
        for (auto& v : voices)
        {
            v.active = false;
            v.phase1 = 0.0;
            v.phase2 = 0.0;
            v.stealFadeGain = 1.0f;
            v.isBeingStolen = false;
            v.ampEnv.prepare(static_cast<float>(sampleRate));
            v.ampEnv.reset();
            v.filterEnv.prepare(static_cast<float>(sampleRate));
            v.filterEnv.reset();
            v.filterLP.reset();
            v.filterLP.setMode(CytomicSVF::Mode::LowPass);
            v.filterHP.reset();
            v.filterHP.setMode(CytomicSVF::Mode::HighPass);
            v.osc1.reset();
            v.osc2.reset();
            for (auto& s : v.superOsc)
                s.reset();
        }

        // LFOs (D005: rate floor 0.01 Hz)
        lfo1.reset();
        lfo2.reset();

        // FX buffers — size dynamically so delay is correct at any sample rate
        // Max delay time is 0.375s (L channel); add 1 sample of headroom.
        kDelayBufSizeDynamic = static_cast<int>(kDelayMaxSeconds * sampleRate) + 1;
        delayBufL.assign(static_cast<size_t>(kDelayBufSizeDynamic), 0.0f);
        delayBufR.assign(static_cast<size_t>(kDelayBufSizeDynamic), 0.0f);
        delayWritePos = 0;
        reverbBuf.fill(0.0f);
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
            v.osc1.reset();
            v.osc2.reset();
            for (auto& s : v.superOsc)
                s.reset();
            v.phase1 = 0.0;
            v.phase2 = 0.0;
            v.stealFadeGain = 1.0f;
            v.isBeingStolen = false;
        }
        lfo1.reset();
        lfo2.reset();
    }

    //-- Audio rendering --------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // ---- MIDI parsing ----
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();

            if (msg.isNoteOn())
            {
                wakeSilenceGate();
                allocateVoice(msg.getNoteNumber(), msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                releaseVoice(msg.getNoteNumber());
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
        const float pHorizon = pHorizonScan ? pHorizonScan->load() : 0.5f;
        const float pParallax = pParallaxAmt ? pParallaxAmt->load() : 0.5f;
        const float pVistaOpen = pVistaLine ? pVistaLine->load() : 0.7f;
        const float pFilterRes = pResonance ? pResonance->load() : 0.3f;
        const float pAtt = pAttack ? pAttack->load() : 0.1f;
        const float pDec = pDecay ? pDecay->load() : 0.3f;
        const float pSus = pSustain ? pSustain->load() : 0.7f;
        const float pRel = pRelease ? pRelease->load() : 0.8f;
        const float pFiltEnvAmt = pFilterEnvAmt ? pFilterEnvAmt->load() : 0.5f;
        const float pLfo1Rate = pLfo1RateParam ? pLfo1RateParam->load() : 0.5f;
        const float pLfo1Dep = pLfo1DepParam ? pLfo1DepParam->load() : 0.3f;
        const float pLfo2Rate = pLfo2RateParam ? pLfo2RateParam->load() : 0.3f;
        const float pLfo2Dep = pLfo2DepParam ? pLfo2DepParam->load() : 0.2f;
        const float pWave1 = pWaveShape1 ? pWaveShape1->load() : 0.0f;
        const float pWave2 = pWaveShape2 ? pWaveShape2->load() : 2.0f;
        const float pOscMix = pOscMixParam ? pOscMixParam->load() : 0.5f;
        const float pReverb = pReverbMix ? pReverbMix->load() : 0.3f;
        const float pDelay = pDelayMix ? pDelayMix->load() : 0.0f;
        const float pMacroChar = pMacroCharacter ? pMacroCharacter->load() : 0.5f;
        const float pMacroMov = pMacroMovement ? pMacroMovement->load() : 0.0f;
        const float pMacroCouple = pMacroCoupling ? pMacroCoupling->load() : 0.0f;
        const float pMacroSpc = pMacroSpace ? pMacroSpace->load() : 0.3f;
        const float pModWheelDep = pModWheelDepth ? pModWheelDepth->load() : 0.5f;
        const float pAfterDep = pAftertouchDep ? pAftertouchDep->load() : 0.5f;

        // ---- LFO tick (block-rate) ----
        lfo1.setRate(pLfo1Rate, static_cast<float>(sr));
        lfo2.setRate(pLfo2Rate, static_cast<float>(sr));

        // ---- Render per-sample ----
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

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
                if (!v.active && !v.ampEnv.isActive())
                    continue;

                ++v.age; // Track voice age for oldest-voice stealing

                // Amp envelope (StandardADSR — exponential decay/release, click-free)
                v.ampEnv.setADSR(pAtt, pDec, pSus, pRel);
                const float ampEnvVal = v.ampEnv.process();
                if (ampEnvVal < 1e-7f && !v.ampEnv.isActive())
                {
                    v.active = false;
                    v.isBeingStolen = false;
                    v.stealFadeGain = 1.0f;
                    continue;
                }

                // Voice-steal crossfade: ramp stealFadeGain → 0 over 5ms
                if (v.isBeingStolen)
                {
                    const float crossfadeRate = 1.0f / (0.005f * static_cast<float>(sr));
                    v.stealFadeGain -= crossfadeRate;
                    if (v.stealFadeGain <= 0.0f)
                    {
                        v.stealFadeGain = 0.0f;
                        v.isBeingStolen = false;
                        v.active = false;
                        v.ampEnv.kill();
                        v.filterEnv.kill();
                        continue;
                    }
                }

                // Filter envelope
                v.filterEnv.setADSR(pAtt * 0.5f, pDec * 0.8f, 0.0f, pRel * 0.5f);
                const float filtEnvVal = v.filterEnv.process();

                // Velocity → timbre (D001): velocity scales filter brightness
                const float velFilterMod = v.velocity * 0.6f + 0.4f;

                // Pitch with bend
                const float freq = v.baseFreq * PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

                // Macro CHARACTER + coupling modulates horizon scan position
                const float cplScaleH = 1.0f + pMacroCouple * 3.0f;
                const float horizonPos =
                    std::clamp(pHorizon + pMacroChar * 0.5f - 0.25f + couplingHorizonMod * cplScaleH, 0.0f, 1.0f);

                // Panorama oscillator: dual wavetable scanning in opposite directions
                const float scan1 = horizonPos;
                const float scan2 = 1.0f - horizonPos;

                const float osc1Sample = renderWavePolyBLEP(v, freq, scan1, pWave1, static_cast<float>(sr), false);
                const float osc2Sample =
                    renderWavePolyBLEP(v, freq * 1.001f, scan2, pWave2, static_cast<float>(sr), true);

                // Advance legacy phase (used by Sine/Formant/Noise cases 0, 6, 7)
                const double phaseInc = freq / sr;
                v.phase1 += phaseInc;
                v.phase2 += phaseInc * 1.001;
                if (v.phase1 >= 1.0)
                    v.phase1 -= 1.0;
                if (v.phase2 >= 1.0)
                    v.phase2 -= 1.0;

                const float oscMix = osc1Sample * (1.0f - pOscMix) + osc2Sample * pOscMix;

                // Vista filter: horizon line opens/closes spectral view
                // COUPLING macro scales all coupling input sensitivity
                const float cplScale = 1.0f + pMacroCouple * 3.0f; // 0→1x, 1→4x
                const float baseCutoff = 200.0f + pVistaOpen * 18000.0f + couplingFilterMod * cplScale;
                const float envCutoff = baseCutoff * (1.0f + filtEnvVal * pFiltEnvAmt * velFilterMod);
                // MOVEMENT macro directly scales LFO→filter depth (no triple attenuation)
                const float movementAmt = pMacroMov + modWheel * pModWheelDep;
                const float modCutoff = envCutoff * (1.0f + lfo1Val * pLfo1Dep * std::max(movementAmt, 0.1f));
                const float finalCutoff = std::clamp(modCutoff + aftertouch * pAfterDep * 4000.0f, 20.0f, 20000.0f);

                v.filterLP.setCoefficients_fast(finalCutoff, pFilterRes, static_cast<float>(sr));
                const float filtered = v.filterLP.processSample(oscMix);

                // HP for clearing low mud based on horizon
                const float hpCutoff = 20.0f + (1.0f - pVistaOpen) * 300.0f;
                v.filterHP.setCoefficients_fast(hpCutoff, 0.5f, static_cast<float>(sr));
                const float cleaned = v.filterHP.processSample(filtered);

                // Aurora luminosity modulates amplitude
                const float luminosity = 1.0f + (auroraLuminosity - 0.5f) * pLfo2Dep * 0.4f;
                // Apply voice-steal crossfade gain to prevent click on voice reassignment
                const float monoOut = cleaned * ampEnvVal * luminosity * v.velocity * v.stealFadeGain;

                // Parallax stereo: high notes spread wider (+ coupling modulation)
                const float noteNorm = (v.note - 36.0f) / 60.0f; // 0=C2, 1=C7
                const float cplScaleP = 1.0f + pMacroCouple * 3.0f;
                const float parallaxTotal = std::clamp(pParallax + couplingParallaxMod * cplScaleP, 0.0f, 1.0f);
                const float spread = std::clamp(noteNorm * parallaxTotal, 0.0f, 1.0f);
                // Equal-power complementary panning
                const float panAngle = spread * 0.4f * (1.0f + lfo2Val * 0.1f);
                const float gainL = std::cos(panAngle * juce::MathConstants<float>::halfPi);
                const float gainR =
                    std::sin(panAngle * juce::MathConstants<float>::halfPi + juce::MathConstants<float>::halfPi * 0.5f);

                mixL += monoOut * gainL;
                mixR += monoOut * gainR;
            }

            // Macro SPACE → reverb/delay depth
            const float spaceAmt = std::clamp(pMacroSpc + pReverb, 0.0f, 1.0f);
            const float delayAmt = std::clamp(pDelay + pMacroCouple * 0.3f, 0.0f, 1.0f);

            // Ping-pong delay (0.375s L, 0.25s R) — uses dynamic buffer sized in prepare()
            const int delayWriteIdx = delayWritePos % kDelayBufSizeDynamic;
            const int delayReadL =
                (delayWritePos - static_cast<int>(0.375 * sr) + kDelayBufSizeDynamic * 4) % kDelayBufSizeDynamic;
            const int delayReadR =
                (delayWritePos - static_cast<int>(0.25 * sr) + kDelayBufSizeDynamic * 4) % kDelayBufSizeDynamic;
            const float delayOutL = flushDenormal(delayBufL[static_cast<size_t>(delayReadL)]);
            const float delayOutR = flushDenormal(delayBufR[static_cast<size_t>(delayReadR)]);
            delayBufL[static_cast<size_t>(delayWriteIdx)] = mixR * 0.5f + delayOutR * 0.35f; // Cross-feed for ping-pong
            delayBufR[static_cast<size_t>(delayWriteIdx)] = mixL * 0.5f + delayOutL * 0.35f;
            delayWritePos = (delayWritePos + 1) % kDelayBufSizeDynamic;

            // Diffusion reverb (4-tap allpass with feedback)
            // Tap offsets are anchored at 44100 Hz and scaled to actual sample rate
            // so reverb density is sample-rate-independent.
            const int revIdx = reverbWritePos % kReverbBufSize;
            const float revIn = (mixL + mixR) * 0.5f;
            float revOut = 0.0f;
            {
                static constexpr int kBaseTaps[4] = {1117, 1543, 2371, 3079};
                const float srScale = static_cast<float>(sr) / 44100.0f;
                for (int t = 0; t < 4; ++t)
                {
                    const int scaledTap = static_cast<int>(kBaseTaps[t] * srScale);
                    const int safeOffset = std::max(1, std::min(scaledTap, kReverbBufSize - 1));
                    const int readIdx = (reverbWritePos - safeOffset + kReverbBufSize * 4) % kReverbBufSize;
                    revOut += reverbBuf[static_cast<size_t>(readIdx)] * 0.25f;
                }
            }
            reverbBuf[static_cast<size_t>(revIdx)] = flushDenormal(revIn + revOut * 0.45f);
            reverbWritePos = (reverbWritePos + 1) % kReverbBufSize;

            outL[s] = mixL + revOut * spaceAmt + delayOutL * delayAmt;
            outR[s] = mixR + revOut * spaceAmt + delayOutR * delayAmt;

            // Denormal protection
            outL[s] = flushDenormal(outL[s]);
            outR[s] = flushDenormal(outR[s]);
        }

        // Coupling buffer
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            juce::FloatVectorOperations::copy(couplingBuffer.getWritePointer(ch), buffer.getReadPointer(ch),
                                              numSamples);

        analyzeForSilenceGate(buffer, numSamples);

        // F04: reset per-block coupling accumulators so stale values don't persist
        // after a coupling route is deactivated (matches OxytocinAdapter pattern).
        couplingFilterMod = 0.0f;
        couplingParallaxMod = 0.0f;
        couplingHorizonMod = 0.0f;
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (channel < couplingBuffer.getNumChannels() && sampleIndex < couplingBuffer.getNumSamples())
            return couplingBuffer.getSample(channel, sampleIndex);
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (std::abs(amount) < 0.001f || sourceBuffer == nullptr)
            return;
        if (numSamples <= 0)
            return; // F03: guard divide-by-zero (legal in tail-render hosts)

        switch (type)
        {
        case CouplingType::AmpToFilter:
        {
            // Source amplitude modulates vista filter cutoff
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += sourceBuffer[i] * sourceBuffer[i];
            rms = std::sqrt(rms / static_cast<float>(numSamples));
            couplingFilterMod = rms * amount * 4000.0f;
            break;
        }
        case CouplingType::LFOToPitch:
        {
            // Source LFO modulates parallax depth
            float avg = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                avg += sourceBuffer[i];
            avg /= static_cast<float>(numSamples);
            couplingParallaxMod = avg * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Source envelope modulates horizon scan
            float peak = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                peak = std::max(peak, std::abs(sourceBuffer[i]));
            couplingHorizonMod = peak * amount;
            break;
        }
        default:
            break;
        }
    }

    //-- Parameters -------------------------------------------------------------

    // W07 fix: addParameters — called by XOceanusProcessor::createParameterLayout()
    // to register all look_ params in the shared APVTS.
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        auto floatParam =
            [&](const juce::String& id, const juce::String& name, float min, float max, float def, float skew = 1.0f)
        {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(min, max, 0.0f, skew), def));
        };

        auto choiceParam = [&](const juce::String& id, const juce::String& name, juce::StringArray choices, int def)
        {
            params.push_back(
                std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, choices, def));
        };

        // Oscillator
        choiceParam("look_waveShape1", "Wave 1",
                    {"Sine", "Triangle", "Saw", "Square", "Pulse", "Super", "Noise", "Formant"}, 0);
        choiceParam("look_waveShape2", "Wave 2",
                    {"Sine", "Triangle", "Saw", "Square", "Pulse", "Super", "Noise", "Formant"}, 2);
        floatParam("look_oscMix", "Osc Mix", 0.0f, 1.0f, 0.5f);
        floatParam("look_horizonScan", "Horizon Scan", 0.0f, 1.0f, 0.5f);

        // Parallax
        floatParam("look_parallaxAmount", "Parallax", 0.0f, 1.0f, 0.5f);

        // Vista filter
        floatParam("look_vistaLine", "Vista Line", 0.0f, 1.0f, 0.7f);
        floatParam("look_resonance", "Resonance", 0.0f, 1.0f, 0.3f);
        floatParam("look_filterEnvAmt", "Filter Env Amt", 0.0f, 1.0f, 0.5f);

        // Envelope
        floatParam("look_attack", "Attack", 0.001f, 5.0f, 0.1f, 0.4f);
        floatParam("look_decay", "Decay", 0.001f, 5.0f, 0.3f, 0.4f);
        floatParam("look_sustain", "Sustain", 0.0f, 1.0f, 0.7f);
        floatParam("look_release", "Release", 0.001f, 10.0f, 0.8f, 0.4f);

        // LFOs (D005: rate floor 0.01 Hz)
        floatParam("look_lfo1Rate", "LFO 1 Rate", 0.01f, 20.0f, 0.5f, 0.3f);
        floatParam("look_lfo1Depth", "LFO 1 Depth", 0.0f, 1.0f, 0.3f);
        floatParam("look_lfo2Rate", "LFO 2 Rate", 0.01f, 20.0f, 0.3f, 0.3f);
        floatParam("look_lfo2Depth", "LFO 2 Depth", 0.0f, 1.0f, 0.2f);

        // Expression (D006)
        floatParam("look_modWheelDepth", "Mod Wheel Depth", 0.0f, 1.0f, 0.5f);
        floatParam("look_aftertouchDepth", "Aftertouch Depth", 0.0f, 1.0f, 0.5f);

        // FX
        floatParam("look_reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.3f);
        floatParam("look_delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f);

        // Macros
        floatParam("look_macroCharacter", "Character", 0.0f, 1.0f, 0.5f);
        floatParam("look_macroMovement", "Movement", 0.0f, 1.0f, 0.0f);
        floatParam("look_macroCoupling", "Coupling", 0.0f, 1.0f, 0.0f);
        floatParam("look_macroSpace", "Space", 0.0f, 1.0f, 0.3f);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        auto floatParam =
            [&](const juce::String& id, const juce::String& name, float min, float max, float def, float skew = 1.0f)
        {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(min, max, 0.0f, skew), def));
        };

        auto choiceParam = [&](const juce::String& id, const juce::String& name, juce::StringArray choices, int def)
        {
            params.push_back(
                std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, choices, def));
        };

        // Oscillator
        choiceParam("look_waveShape1", "Wave 1",
                    {"Sine", "Triangle", "Saw", "Square", "Pulse", "Super", "Noise", "Formant"}, 0);
        choiceParam("look_waveShape2", "Wave 2",
                    {"Sine", "Triangle", "Saw", "Square", "Pulse", "Super", "Noise", "Formant"}, 2);
        floatParam("look_oscMix", "Osc Mix", 0.0f, 1.0f, 0.5f);
        floatParam("look_horizonScan", "Horizon Scan", 0.0f, 1.0f, 0.5f);

        // Parallax
        floatParam("look_parallaxAmount", "Parallax", 0.0f, 1.0f, 0.5f);

        // Vista filter
        floatParam("look_vistaLine", "Vista Line", 0.0f, 1.0f, 0.7f);
        floatParam("look_resonance", "Resonance", 0.0f, 1.0f, 0.3f);
        floatParam("look_filterEnvAmt", "Filter Env Amt", 0.0f, 1.0f, 0.5f);

        // Envelope
        floatParam("look_attack", "Attack", 0.001f, 5.0f, 0.1f, 0.4f);
        floatParam("look_decay", "Decay", 0.001f, 5.0f, 0.3f, 0.4f);
        floatParam("look_sustain", "Sustain", 0.0f, 1.0f, 0.7f);
        floatParam("look_release", "Release", 0.001f, 10.0f, 0.8f, 0.4f);

        // LFOs (D005: rate floor 0.01 Hz)
        floatParam("look_lfo1Rate", "LFO 1 Rate", 0.01f, 20.0f, 0.5f, 0.3f);
        floatParam("look_lfo1Depth", "LFO 1 Depth", 0.0f, 1.0f, 0.3f);
        floatParam("look_lfo2Rate", "LFO 2 Rate", 0.01f, 20.0f, 0.3f, 0.3f);
        floatParam("look_lfo2Depth", "LFO 2 Depth", 0.0f, 1.0f, 0.2f);

        // Expression (D006)
        floatParam("look_modWheelDepth", "Mod Wheel Depth", 0.0f, 1.0f, 0.5f);
        floatParam("look_aftertouchDepth", "Aftertouch Depth", 0.0f, 1.0f, 0.5f);

        // FX
        floatParam("look_reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.3f);
        floatParam("look_delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f);

        // Macros
        floatParam("look_macroCharacter", "Character", 0.0f, 1.0f, 0.5f);
        floatParam("look_macroMovement", "Movement", 0.0f, 1.0f, 0.0f);
        floatParam("look_macroCoupling", "Coupling", 0.0f, 1.0f, 0.0f);
        floatParam("look_macroSpace", "Space", 0.0f, 1.0f, 0.3f);

        return {params.begin(), params.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pHorizonScan = apvts.getRawParameterValue("look_horizonScan");
        pParallaxAmt = apvts.getRawParameterValue("look_parallaxAmount");
        pVistaLine = apvts.getRawParameterValue("look_vistaLine");
        pResonance = apvts.getRawParameterValue("look_resonance");
        pFilterEnvAmt = apvts.getRawParameterValue("look_filterEnvAmt");
        pAttack = apvts.getRawParameterValue("look_attack");
        pDecay = apvts.getRawParameterValue("look_decay");
        pSustain = apvts.getRawParameterValue("look_sustain");
        pRelease = apvts.getRawParameterValue("look_release");
        pLfo1RateParam = apvts.getRawParameterValue("look_lfo1Rate");
        pLfo1DepParam = apvts.getRawParameterValue("look_lfo1Depth");
        pLfo2RateParam = apvts.getRawParameterValue("look_lfo2Rate");
        pLfo2DepParam = apvts.getRawParameterValue("look_lfo2Depth");
        pWaveShape1 = apvts.getRawParameterValue("look_waveShape1");
        pWaveShape2 = apvts.getRawParameterValue("look_waveShape2");
        pOscMixParam = apvts.getRawParameterValue("look_oscMix");
        pReverbMix = apvts.getRawParameterValue("look_reverbMix");
        pDelayMix = apvts.getRawParameterValue("look_delayMix");
        pMacroCharacter = apvts.getRawParameterValue("look_macroCharacter");
        pMacroMovement = apvts.getRawParameterValue("look_macroMovement");
        pMacroCoupling = apvts.getRawParameterValue("look_macroCoupling");
        pMacroSpace = apvts.getRawParameterValue("look_macroSpace");
        pModWheelDepth = apvts.getRawParameterValue("look_modWheelDepth");
        pAftertouchDep = apvts.getRawParameterValue("look_aftertouchDepth");
    }

private:
    //-- Voice ------------------------------------------------------------------

    struct Voice
    {
        bool active = false;
        int note = 60;
        float velocity = 0.0f;
        float baseFreq = 440.0f;
        // phase1/phase2 retained for sine, noise and formant cases (0, 6, 7)
        // that bypass PolyBLEP and use the legacy phase accumulator directly.
        double phase1 = 0.0, phase2 = 0.0;
        uint32_t age = 0; // For oldest-voice stealing
        // F06: per-voice PRNG seed (was static thread_local — hidden RT alloc on first access).
        // Each voice gets independent noise (sonically better too).
        uint32_t noiseSeed = 2463534242u;
        // StandardADSR replaces SimpleEnvelope — exponential decay/release, no clicks.
        StandardADSR ampEnv, filterEnv;
        CytomicSVF filterLP, filterHP;
        // Per-voice PolyBLEP oscillators (antialiased saw/square/pulse/triangle/super).
        // osc2 is detuned by 1.001x for parallax width; superOsc[0..2] for the super stack.
        PolyBLEP osc1, osc2;
        PolyBLEP superOsc[3]; // detuned copies for "Super" mode (case 5)
        // Voice-steal crossfade: ramps from 1.0 → 0 at steal time to prevent clicks.
        float stealFadeGain = 1.0f;
        bool isBeingStolen = false;
    };

    static constexpr int kMaxVoices = 8;
    std::array<Voice, kMaxVoices> voices{};

    //-- Voice management -------------------------------------------------------

    void allocateVoice(int noteNumber, float vel)
    {
        // Read current envelope params for initial ADSR configuration.
        const float pAtt = pAttack ? pAttack->load() : 0.1f;
        const float pDec = pDecay ? pDecay->load() : 0.3f;
        const float pSus = pSustain ? pSustain->load() : 0.7f;
        const float pRel = pRelease ? pRelease->load() : 0.8f;

        // Find free voice (not active, envelope idle)
        int idx = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[static_cast<size_t>(i)].active && !voices[static_cast<size_t>(i)].ampEnv.isActive())
            {
                idx = i;
                break;
            }
        }
        if (idx == -1)
        {
            // Steal oldest active voice — start a 5ms crossfade on it first
            uint32_t maxAge = 0;
            idx = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[static_cast<size_t>(i)].age > maxAge)
                {
                    maxAge = voices[static_cast<size_t>(i)].age;
                    idx = i;
                }
            }
            // Mark old voice for crossfade; capture level before reset.
            // The render loop will ramp stealFadeGain → 0 over 5ms (new note
            // plays at reduced amplitude during the crossfade window, preventing click).
            voices[static_cast<size_t>(idx)].isBeingStolen = true;
            voices[static_cast<size_t>(idx)].stealFadeGain = voices[static_cast<size_t>(idx)].ampEnv.getLevel();
        }

        auto& v = voices[static_cast<size_t>(idx)];
        // Preserve crossfade state if this slot was just marked as stolen.
        const bool wasStolen = v.isBeingStolen;
        const float stolenGain = v.stealFadeGain;

        v.active = true;
        v.note = noteNumber;
        v.velocity = vel;
        v.baseFreq = 440.0f * std::pow(2.0f, (noteNumber - 69.0f) / 12.0f);
        v.phase1 = 0.0;
        v.phase2 = 0.0;
        v.age = 0;
        // Restore crossfade state so the render loop fades the new note in cleanly.
        v.isBeingStolen = wasStolen;
        v.stealFadeGain = wasStolen ? stolenGain : 1.0f;

        // Configure StandardADSR
        v.ampEnv.prepare(static_cast<float>(sr));
        v.ampEnv.setADSR(pAtt, pDec, pSus, pRel);
        v.ampEnv.noteOn();

        v.filterEnv.prepare(static_cast<float>(sr));
        v.filterEnv.setADSR(pAtt * 0.5f, pDec * 0.8f, 0.0f, pRel * 0.5f);
        v.filterEnv.noteOn();

        // Reset per-voice PolyBLEP oscillators and configure frequency.
        // Waveform is set per-sample in renderWavePolyBLEP.
        v.osc1.reset();
        v.osc1.setFrequency(v.baseFreq, static_cast<float>(sr));
        v.osc2.reset();
        v.osc2.setFrequency(v.baseFreq * 1.001f, static_cast<float>(sr));
        for (int d = 0; d < 3; ++d)
        {
            v.superOsc[d].reset();
            const float detuneFactor = 1.0f + (d - 1) * 0.005f;
            v.superOsc[d].setFrequency(v.baseFreq * detuneFactor, static_cast<float>(sr));
            v.superOsc[d].setWaveform(PolyBLEP::Waveform::Saw);
        }

        ++voiceCounter;
    }

    void releaseVoice(int noteNumber)
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == noteNumber)
            {
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
                v.active = false;
                break;
            }
        }
    }

    //-- Wavetable rendering ----------------------------------------------------

    // renderWavePolyBLEP — uses per-voice PolyBLEP objects for band-limited
    // waveforms (cases 1–5) and falls back to the legacy phase-based approach
    // for bandlimited-by-nature waveforms (sine, noise, formant — cases 0, 6, 7).
    //
    // @param v        Voice reference (PolyBLEP oscillators are mutated here)
    // @param freq     Frequency in Hz (already pitch-bent)
    // @param scan     Horizon scan position [0,1] controlling morph/width
    // @param pWave    Wave shape selector (float cast from int param)
    // @param srf      Sample rate as float
    // @param useOsc2  True = use v.osc2 (detuned layer), False = use v.osc1
    float renderWavePolyBLEP(Voice& v, float freq, float scan, float pWave, float srf, bool useOsc2) noexcept
    {
        const int shape = static_cast<int>(pWave);
        const float s = std::clamp(scan, 0.0f, 1.0f);
        PolyBLEP& osc = useOsc2 ? v.osc2 : v.osc1;

        switch (shape)
        {
        case 0: // Sine → scan adds odd harmonics (sine is already BL — no PolyBLEP needed)
        {
            const float p = static_cast<float>(useOsc2 ? v.phase2 : v.phase1);
            const float sine = fastSin(p * juce::MathConstants<float>::twoPi);
            const float h3 = fastSin(p * juce::MathConstants<float>::twoPi * 3.0f) * 0.333f;
            const float h5 = fastSin(p * juce::MathConstants<float>::twoPi * 5.0f) * 0.2f;
            float base = sine + s * (h3 + h5);
            base /= (1.0f + s * 0.533f);
            return base;
        }

        case 1: // Triangle — PolyBLEP antialiased; scan morphs toward saw via blend
        {
            osc.setFrequency(freq, srf);
            osc.setWaveform(PolyBLEP::Waveform::Triangle);
            const float tri = osc.processSample();
            if (s > 0.001f)
            {
                // superOsc[0] used as scratch PolyBLEP Saw for triangle→saw morph.
                // (superOsc slots are only meaningful when shape==5; safe to borrow here.)
                v.superOsc[0].setFrequency(freq, srf);
                v.superOsc[0].setWaveform(PolyBLEP::Waveform::Saw);
                const float saw = v.superOsc[0].processSample();
                return tri * (1.0f - s) + saw * s;
            }
            return tri;
        }

        case 2: // Saw — PolyBLEP antialiased; scan adds 2nd-harmonic brightness blend
        {
            osc.setFrequency(freq, srf);
            osc.setWaveform(PolyBLEP::Waveform::Saw);
            const float saw = osc.processSample();
            if (s > 0.001f)
            {
                const float p = static_cast<float>(useOsc2 ? v.phase2 : v.phase1);
                const float h2 = fastSin(p * juce::MathConstants<float>::twoPi * 2.0f) * 0.5f;
                return (saw + s * h2) / (1.0f + s * 0.5f);
            }
            return saw;
        }

        case 3: // Square — PolyBLEP Pulse; scan morphs pulse width 50%→10%
        {
            const float pw = 0.5f - s * 0.4f; // 0.5 at scan=0, 0.1 at scan=1
            osc.setFrequency(freq, srf);
            osc.setWaveform(PolyBLEP::Waveform::Pulse);
            osc.setPulseWidth(pw);
            return osc.processSample();
        }

        case 4: // Pulse — PolyBLEP antialiased; variable width via scan
        {
            const float pw = 0.1f + s * 0.8f;
            osc.setFrequency(freq, srf);
            osc.setWaveform(PolyBLEP::Waveform::Pulse);
            osc.setPulseWidth(pw);
            return osc.processSample();
        }

        case 5: // Super — 3 detuned PolyBLEP saws; scan controls detune depth
        {
            float sum = 0.0f;
            for (int d = 0; d < 3; ++d)
            {
                const float detuneFactor = 1.0f + (d - 1) * 0.005f * (1.0f + s);
                v.superOsc[d].setFrequency(freq * detuneFactor, srf);
                v.superOsc[d].setWaveform(PolyBLEP::Waveform::Saw);
                sum += v.superOsc[d].processSample();
            }
            return sum / 3.0f;
        }

        case 6: // Noise — xorshift32 PRNG (wideband, no PolyBLEP needed)
        {
            v.noiseSeed ^= v.noiseSeed << 13;
            v.noiseSeed ^= v.noiseSeed >> 17;
            v.noiseSeed ^= v.noiseSeed << 5;
            return static_cast<float>(v.noiseSeed) / static_cast<float>(0xFFFFFFFFu) * 2.0f - 1.0f;
        }

        case 7: // Formant — sine product (naturally BL, no PolyBLEP needed)
        {
            const float p = static_cast<float>(useOsc2 ? v.phase2 : v.phase1);
            const float formantFreq = 1.0f + s * 4.0f;
            return fastSin(p * juce::MathConstants<float>::twoPi) *
                   fastSin(p * juce::MathConstants<float>::twoPi * formantFreq);
        }

        default:
        {
            const float p = static_cast<float>(useOsc2 ? v.phase2 : v.phase1);
            return fastSin(p * juce::MathConstants<float>::twoPi);
        }
        }
    }

    //-- Utility ----------------------------------------------------------------

    static float midiToFreq(float note) { return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f); }

    //-- State ------------------------------------------------------------------

    double sr = 48000.0;
    int blockSize = 512;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBendNorm = 0.0f;
    uint32_t voiceCounter = 0;

    // Delay buffers (ping-pong) — dynamically sized in prepare() for sample-rate independence.
    // Max delay time = 0.5s (covers 0.375s L + headroom at any sample rate up to 192 kHz+).
    static constexpr float kDelayMaxSeconds = 0.5f;
    int kDelayBufSizeDynamic = 24000; // updated in prepare()
    std::vector<float> delayBufL{};
    std::vector<float> delayBufR{};
    int delayWritePos = 0;

    // Reverb buffer (4-tap allpass diffusion)
    static constexpr int kReverbBufSize = 16384; // supports up to ~176kHz (largest tap 3079 * 4x)
    std::array<float, kReverbBufSize> reverbBuf{};
    int reverbWritePos = 0;

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingParallaxMod = 0.0f;
    float couplingHorizonMod = 0.0f;

    juce::AudioBuffer<float> couplingBuffer;

    StandardLFO lfo1, lfo2;

    // Cached parameter pointers
    std::atomic<float>* pHorizonScan = nullptr;
    std::atomic<float>* pParallaxAmt = nullptr;
    std::atomic<float>* pVistaLine = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pFilterEnvAmt = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLfo1RateParam = nullptr;
    std::atomic<float>* pLfo1DepParam = nullptr;
    std::atomic<float>* pLfo2RateParam = nullptr;
    std::atomic<float>* pLfo2DepParam = nullptr;
    std::atomic<float>* pWaveShape1 = nullptr;
    std::atomic<float>* pWaveShape2 = nullptr;
    std::atomic<float>* pOscMixParam = nullptr;
    std::atomic<float>* pReverbMix = nullptr;
    std::atomic<float>* pDelayMix = nullptr;
    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;
    std::atomic<float>* pModWheelDepth = nullptr;
    std::atomic<float>* pAftertouchDep = nullptr;
};

} // namespace xoceanus
