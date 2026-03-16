#pragma once

//==============================================================================
// XOverlapAdapter.h — XOmnibus adapter for XOverlap (OVERLAP)
//
// 6-voice knot-topology FDN synthesizer.
// Lion's Mane jellyfish signal tangling through Feedback Delay Networks.
// Knot types (Unknot / Trefoil / Figure-Eight / Torus) reconfigure the FDN
// routing matrix. Voices entrain via hydrodynamic Kuramoto pulse coupling.
//
// 41 canonical olap_ parameters. Gallery code: OVERLAP.
// Accent: Bioluminescent Cyan-Green #00FFB4
//
// XOmnibus integration: DSP headers resolved via target_include_directories
// pointing to XOverlap/Source/ — see CMakeLists.txt.
//==============================================================================

#include "../../Core/SynthEngine.h"

// XOverlap DSP — included via target_include_directories in CMakeLists.txt
// (path: XOverlap/Source/DSP/)
#include "DSP/Voice.h"
#include "DSP/FDN.h"
#include "DSP/Entrainment.h"
#include "DSP/Bioluminescence.h"
#include "DSP/PostFX.h"
#include "DSP/ParamSnapshot.h"
#include "DSP/KnotMatrix.h"
#include "DSP/FastMath.h"
#include "../../DSP/FastMath.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace xomnibus {

//==============================================================================
class XOverlapEngine : public SynthEngine
{
public:
    //==========================================================================
    void prepare(double sampleRate, int /*maxBlockSize*/) override
    {
        sr = sampleRate;

        for (int i = 0; i < kVoices; ++i)
        {
            voices[static_cast<size_t>(i)].prepare(sampleRate);
            voices[static_cast<size_t>(i)].setVoiceIndex(i);
        }

        fdn.prepare(sampleRate);
        entrainment.prepare(sampleRate);
        biolum.prepare(sampleRate);
        postFx.prepare(sampleRate);

        svfIcL[0] = svfIcL[1] = 0.0f;
        svfIcR[0] = svfIcR[1] = 0.0f;
        filterEnvLevel    = 0.0f;
        filterEnvDecayRate = 0.0f;
        filterEnvVelocity = 0.0f;
        lfo1Phase  = 0.0f;
        lfo2Phase  = 0.0f;
        lfo1SHValue = 0.0f;
        lfo2SHValue = 0.0f;
        noteOrderCounter = 0;
        previousKnot = -1;
        delayRatios.fill(1.0f);

        extPitchMod  = 0.0f;
        extFilterMod = 0.0f;
        extRingMod   = 0.0f;
        extDelayMod  = 0.0f;

        lastSampleL = lastSampleR = 0.0f;
        activeCount = 0;
    }

    void releaseResources() override
    {
        for (auto& v : voices)
            v.reset();
    }

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        svfIcL[0] = svfIcL[1] = 0.0f;
        svfIcR[0] = svfIcR[1] = 0.0f;
        filterEnvLevel = 0.0f;
        lfo1Phase = lfo2Phase = 0.0f;
        lfo1SHValue = lfo2SHValue = 0.0f;
        lastSampleL = lastSampleR = 0.0f;
    }

    //==========================================================================
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Cache params once per block — use stored apvts pointer
        if (apvts_ptr != nullptr)
            params.update(*apvts_ptr);

        // Update envelope params on all active voices
        for (auto& v : voices)
            if (v.isActive())
                v.env.setParams(params.attack, params.decay, params.sustain, params.release);

        // Modulated working copies for this block
        float modTangleDepth     = params.tangleDepth;
        float modDampening       = params.dampening;
        float modPulseRate       = params.pulseRate;
        float modDelayBase       = params.delayBase;
        float modFilterCutoff    = params.filterCutoff;
        float modSpread          = params.spread;
        float modKnot            = static_cast<float>(params.knot);
        float modEntrain         = params.entrain;
        float modFeedback        = params.feedback;
        float modBioluminescence = params.bioluminescence;

        // LFO processing
        float invSr    = 1.0f / static_cast<float>(sr);
        float lfo1Inc  = params.lfo1Rate * invSr;
        float lfo2Inc  = params.lfo2Rate * invSr;
        float lfo1Val  = computeLFO(lfo1Phase, params.lfo1Shape, lfo1SHValue);
        float lfo2Val  = computeLFO(lfo2Phase, params.lfo2Shape, lfo2SHValue);

        if (params.lfo1Depth > 0.001f)
            applyLFOModulation(lfo1Val, params.lfo1Depth, params.lfo1Dest,
                               modTangleDepth, modDampening, modPulseRate,
                               modDelayBase, modFilterCutoff, modSpread);
        if (params.lfo2Depth > 0.001f)
            applyLFOModulation(lfo2Val, params.lfo2Depth, params.lfo2Dest,
                               modTangleDepth, modDampening, modPulseRate,
                               modDelayBase, modFilterCutoff, modSpread);

        // Apply macros
        {
            float mk = params.macroKnot;
            if      (mk < 0.25f) { modKnot = 0.0f; modTangleDepth = mk / 0.25f * 0.8f; }
            else if (mk < 0.33f) { modKnot = 0.0f; modTangleDepth = 0.8f; }
            else if (mk < 0.50f) { modKnot = 1.0f; modTangleDepth = 0.3f + (mk - 0.33f) / 0.17f * 0.4f; }
            else if (mk < 0.66f) { modKnot = 1.0f; modTangleDepth = 0.7f; }
            else if (mk < 0.80f) { modKnot = 2.0f; modTangleDepth = 0.5f + (mk - 0.66f) / 0.14f * 0.4f; }
            else if (mk < 0.90f) { modKnot = 2.0f; modTangleDepth = 0.9f; }
            else                  { modKnot = 3.0f; modTangleDepth = 0.6f + (mk - 0.90f) / 0.10f * 0.4f; }

            float mp = params.macroPulse;
            modPulseRate = 0.01f + mp * (8.0f - 0.01f);
            modSpread    = 0.3f  + mp * 0.7f;

            float me = params.macroEntrain;
            modEntrain  = me;
            modFeedback = 0.3f + me * 0.65f;

            float mb = params.macroBloom;
            modBioluminescence = mb * 0.8f;
            modFilterCutoff    = std::clamp(modFilterCutoff * (1.0f + mb), 20.0f, 20000.0f);
        }

        // Apply expression routing
        {
            float mwMod = modWheelValue * params.modWheelDepth;
            switch (params.modWheelDest)
            {
                case 0: modTangleDepth = std::clamp(modTangleDepth + mwMod, 0.0f, 1.0f); break;
                case 1: modEntrain     = std::clamp(modEntrain + mwMod, 0.0f, 1.0f); break;
                case 2: modBioluminescence = std::clamp(modBioluminescence + mwMod, 0.0f, 1.0f); break;
                case 3: modFilterCutoff = std::clamp(modFilterCutoff * fastPow2(mwMod * 3.0f), 20.0f, 20000.0f); break;
                default: break;
            }
            float atMod = aftertouchValue * params.atPressureDepth;
            switch (params.atPressureDest)
            {
                case 0: modTangleDepth = std::clamp(modTangleDepth + atMod, 0.0f, 1.0f); break;
                case 1: modEntrain     = std::clamp(modEntrain + atMod, 0.0f, 1.0f); break;
                case 2: modBioluminescence = std::clamp(modBioluminescence + atMod, 0.0f, 1.0f); break;
                case 3: modPulseRate   = std::max(0.01f, modPulseRate * (1.0f + atMod)); break;
                default: break;
            }
        }

        // Apply coupling inputs
        modDelayBase   = std::max(1.0f, modDelayBase * (1.0f + extDelayMod * 0.3f));
        modFilterCutoff = std::clamp(modFilterCutoff + extFilterMod, 20.0f, 20000.0f);
        modTangleDepth  = std::clamp(modTangleDepth + extPitchMod * 0.05f, 0.0f, 1.0f);

        // Update FDN matrix based on knot type + tangle depth
        xoverlap::KnotMatrix::Matrix knotMat;
        switch (static_cast<int>(modKnot))
        {
            case 0:  knotMat = xoverlap::KnotMatrix::unknot(); break;
            case 1:  knotMat = xoverlap::KnotMatrix::trefoil(); break;
            case 2:  knotMat = xoverlap::KnotMatrix::figureEight(); break;
            case 3:  knotMat = xoverlap::KnotMatrix::torus(params.torusP, params.torusQ); break;
            default: knotMat = xoverlap::KnotMatrix::unknot(); break;
        }
        auto effectiveMatrix = xoverlap::KnotMatrix::interpolate(knotMat, modTangleDepth);
        fdn.setMatrix(effectiveMatrix);
        fdn.feedback       = modFeedback;
        fdn.dampeningCoeff = params.dampening;

        // Delay ratios per knot type
        int modKnotInt = static_cast<int>(modKnot);
        if (modKnotInt == 3)
            delayRatios = xoverlap::KnotMatrix::torusRatios(params.torusP, params.torusQ);
        else if (modKnotInt == 2)
            delayRatios = { 1.0f, 0.93f, 1.07f, 0.86f, 1.14f, 1.0f };
        else
            delayRatios.fill(1.0f);

        fdn.setDelayBase(modDelayBase, sr, delayRatios);

        // SVF filter coefficients (once per block)
        float envMod = filterEnvLevel * filterEnvVelocity * params.filterEnvAmt;
        float effectiveCutoff = modFilterCutoff * fastPow2(envMod * 4.0f);
        effectiveCutoff = std::clamp(effectiveCutoff, 20.0f,
                                     std::min(20000.0f, static_cast<float>(sr) * 0.45f));
        float g_svf   = fastTan(3.14159265f * effectiveCutoff / static_cast<float>(sr));
        float k_svf   = 2.0f - 2.0f * params.filterRes;
        k_svf         = std::max(k_svf, 0.01f);
        float svfDen  = 1.0f / (1.0f + k_svf * g_svf + g_svf * g_svf);

        // Get write pointers
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        auto midiIt = midi.cbegin();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Handle MIDI at this sample position
            while (midiIt != midi.cend())
            {
                const auto metadata = *midiIt;
                if (metadata.samplePosition > sample) break;

                auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                    handleNoteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff())
                    handleNoteOff(msg.getNoteNumber());
                else if (msg.isController() && msg.getControllerNumber() == 1)
                    modWheelValue = msg.getControllerValue() / 127.0f;
                else if (msg.isChannelPressure())
                    aftertouchValue = msg.getChannelPressureValue() / 127.0f;
                ++midiIt;
            }

            // a. Process each active voice
            std::array<float, kVoices> fdnInputs{};
            for (int i = 0; i < kVoices; ++i)
            {
                auto& v = voices[static_cast<size_t>(i)];
                if (v.isActive())
                    fdnInputs[static_cast<size_t>(i)] = v.process(modPulseRate);
            }

            // b. Process FDN
            fdn.process(fdnInputs);

            // c. Kuramoto entrainment update
            entrainment.process(voices, modEntrain, modPulseRate);

            // d. Stereo spread from FDN outputs
            float left    = 0.0f;
            float right   = 0.0f;
            float fdnMono = 0.0f;

            for (int i = 0; i < kVoices; ++i)
            {
                float fdnOut = fdn.getOutput(i);
                fdnMono += fdnOut;

                float pan     = kPanPositions[i] * modSpread;
                float panNorm = (pan + 1.0f) * 0.5f;
                float lGain   = fastCos(panNorm * 1.5707963f);
                float rGain   = fastSin(panNorm * 1.5707963f);
                left  += fdnOut * lGain;
                right += fdnOut * rGain;
            }
            fdnMono *= (1.0f / static_cast<float>(kVoices));

            // e. Bioluminescence layer
            float bioSample = biolum.process(fdnMono, modDelayBase, modBioluminescence);
            left  += bioSample;
            right += bioSample;

            // f. SVF lowpass filter (Zavalishin TPT)
            {
                float hpL = (left - (k_svf + g_svf) * svfIcL[0] - svfIcL[1]) * svfDen;
                float bpL = g_svf * hpL + svfIcL[0];
                float lpL = g_svf * bpL + svfIcL[1];
                svfIcL[0] = xoverlap::flushDenormal(2.0f * bpL - svfIcL[0]);
                svfIcL[1] = xoverlap::flushDenormal(2.0f * lpL - svfIcL[1]);
                left = lpL;
            }
            {
                float hpR = (right - (k_svf + g_svf) * svfIcR[0] - svfIcR[1]) * svfDen;
                float bpR = g_svf * hpR + svfIcR[0];
                float lpR = g_svf * bpR + svfIcR[1];
                svfIcR[0] = xoverlap::flushDenormal(2.0f * bpR - svfIcR[0]);
                svfIcR[1] = xoverlap::flushDenormal(2.0f * lpR - svfIcR[1]);
                right = lpR;
            }

            // g. Post FX (chorus + diffusion)
            postFx.process(left, right, params.chorusRate, params.chorusMix, params.diffusion);

            // h. Apply coupling ring modulation
            if (extRingMod != 0.0f)
            {
                left  *= (1.0f + extRingMod);
                right *= (1.0f + extRingMod);
            }

            // i. Accumulate into buffer (XOmnibus handles clearing — never overwrite)
            outL[sample] += left;
            if (outR != nullptr)
                outR[sample] += right;

            lastSampleL = left;
            lastSampleR = right;

            // Advance filter envelope
            if (filterEnvLevel > 0.0001f)
                filterEnvLevel -= filterEnvLevel * filterEnvDecayRate;
            else
                filterEnvLevel = 0.0f;

            // Advance LFO phases
            lfo1Phase += lfo1Inc;
            if (lfo1Phase >= 1.0f)
            {
                lfo1Phase -= 1.0f;
                if (params.lfo1Shape == 4) lfo1SHValue = lfoNoise();
            }
            lfo2Phase += lfo2Inc;
            if (lfo2Phase >= 1.0f)
            {
                lfo2Phase -= 1.0f;
                if (params.lfo2Shape == 4) lfo2SHValue = lfoNoise();
            }
        }

        // Count active voices for XOmnibus voice display
        activeCount = 0;
        for (auto& v : voices)
            if (v.isActive()) ++activeCount;
    }

    //==========================================================================
    float getSampleForCoupling(int ch, int /*sample*/) const override
    {
        return ch == 0 ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput(CouplingType t, float amount,
                            const float* buf, int /*ns*/) override
    {
        switch (t)
        {
            case CouplingType::AudioToFM:
                // Treat as FM modulation of FDN delay base
                extDelayMod = (buf ? buf[0] : 0.0f) * amount;
                break;
            case CouplingType::AudioToRing:
                // Ring modulate output
                extRingMod = (buf ? buf[0] : 0.0f) * amount;
                break;
            case CouplingType::AmpToFilter:
                // Raise filter cutoff proportionally to input amplitude
                extFilterMod = amount * 4000.0f;
                break;
            case CouplingType::EnvToMorph:
                // Treat as tangle depth push (push topology toward more tangled)
                extPitchMod = amount;
                break;
            case CouplingType::LFOToPitch:
                // Pitch modulation in semitones routes to tangle depth perturbation
                extPitchMod = (buf ? buf[0] : 0.0f) * amount;
                break;
            case CouplingType::PitchToPitch:
                // Same as LFO — semitone offset becomes tangle perturbation
                extPitchMod = (buf ? buf[0] : 0.0f) * amount;
                break;
            case CouplingType::FilterToFilter:
                // Multiplicative filter cutoff shift
                extFilterMod = amount * modFilterCutoffCache;
                break;
            default:
                break;
        }
    }

    //==========================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

        // Topology
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_knot", "Knot Type",
            juce::StringArray{"Unknot", "Trefoil", "Figure-Eight", "Torus"}, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_tangleDepth", "Tangle Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        p.push_back(std::make_unique<juce::AudioParameterInt>("olap_torusP", "Torus P", 2, 7, 3));
        p.push_back(std::make_unique<juce::AudioParameterInt>("olap_torusQ", "Torus Q", 2, 7, 2));

        // Voice
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_pulseRate", "Pulse Rate",
            juce::NormalisableRange<float>(0.01f, 8.0f, 0.0f, 0.3f), 0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_entrain", "Entrainment",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_spread", "Voice Spread",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_voiceMode", "Voice Mode",
            juce::StringArray{"Poly", "Mono", "Legato"}, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_glide", "Glide Time",
            juce::NormalisableRange<float>(0.0f, 500.0f, 0.0f, 0.4f), 0.0f));

        // FDN
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_delayBase", "Delay Base",
            juce::NormalisableRange<float>(1.0f, 50.0f, 0.0f, 0.4f), 10.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_dampening", "Dampening",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_feedback", "Feedback",
            juce::NormalisableRange<float>(0.0f, 0.99f), 0.7f));

        // Timbre
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_brightness", "Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_bioluminescence", "Bioluminescence",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_current", "Ocean Current",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_currentRate", "Current Rate",
            juce::NormalisableRange<float>(0.005f, 0.5f, 0.0f, 0.3f), 0.03f));

        // Envelope
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_attack", "Attack",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.05f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_decay", "Decay",
            juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.3f), 1.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_sustain", "Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_release", "Release",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 2.0f));

        // Filter
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_filterCutoff", "Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.25f), 8000.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_filterRes", "Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_filterEnvAmt", "Filter Env Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f), 0.3f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_filterEnvDecay", "Filter Env Decay",
            juce::NormalisableRange<float>(0.01f, 5.0f, 0.0f, 0.3f), 0.5f));

        // LFOs
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_lfo1Rate", "LFO 1 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.3f), 0.8f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_lfo1Shape", "LFO 1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_lfo1Depth", "LFO 1 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_lfo1Dest", "LFO 1 Dest",
            juce::StringArray{"Tangle", "Dampening", "Pulse Rate", "Delay", "Filter", "Spread"}, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_lfo2Rate", "LFO 2 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.3f), 0.15f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_lfo2Shape", "LFO 2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_lfo2Depth", "LFO 2 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_lfo2Dest", "LFO 2 Dest",
            juce::StringArray{"Tangle", "Dampening", "Pulse Rate", "Delay", "Filter", "Spread"}, 2));

        // Post FX
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_chorusMix", "Chorus Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_chorusRate", "Chorus Rate",
            juce::NormalisableRange<float>(0.01f, 0.5f, 0.0f, 0.3f), 0.08f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_diffusion", "Diffusion",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Macros
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_macroKnot", "M1 KNOT",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_macroPulse", "M2 PULSE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_macroEntrain", "M3 ENTRAIN",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_macroBloom", "M4 BLOOM",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Expression
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_modWheelDest", "Mod Wheel Dest",
            juce::StringArray{"Tangle", "Entrain", "Bioluminescence", "Filter"}, 0));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_modWheelDepth", "Mod Wheel Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        p.push_back(std::make_unique<juce::AudioParameterChoice>("olap_atPressureDest", "Aftertouch Dest",
            juce::StringArray{"Tangle", "Entrain", "Brightness", "Pulse Rate"}, 1));
        p.push_back(std::make_unique<juce::AudioParameterFloat>("olap_atPressureDepth", "Aftertouch Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        return { p.begin(), p.end() };
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        apvts_ptr = &apvts;
    }

    //==========================================================================
    juce::String getEngineId()      const override { return "XOverlap"; }
    juce::Colour getAccentColour()  const override { return juce::Colour(0xFF00FFB4); }
    int getMaxVoices()              const override { return kVoices; }
    int getActiveVoiceCount()       const override { return activeCount; }

private:
    //==========================================================================
    static constexpr int kVoices = 6;
    static constexpr float kPanPositions[6] = { -1.0f, -0.6f, -0.2f, 0.2f, 0.6f, 1.0f };

    // APVTS pointer — set in attachParameters(), used in renderBlock()
    juce::AudioProcessorValueTreeState* apvts_ptr = nullptr;

    // DSP components
    std::array<xoverlap::Voice, kVoices> voices;
    xoverlap::FeedbackDelayNetwork fdn;
    xoverlap::Entrainment          entrainment;
    xoverlap::Bioluminescence      biolum;
    xoverlap::PostFX               postFx;
    xoverlap::ParamSnapshot        params;

    // SVF filter state (post-FDN Zavalishin TPT lowpass)
    float svfIcL[2] = { 0.0f, 0.0f };
    float svfIcR[2] = { 0.0f, 0.0f };

    // Filter envelope state
    float filterEnvLevel     = 0.0f;
    float filterEnvDecayRate = 0.0f;
    float filterEnvVelocity  = 0.0f;

    // LFO state
    float    lfo1Phase   = 0.0f;
    float    lfo2Phase   = 0.0f;
    float    lfo1SHValue = 0.0f;
    float    lfo2SHValue = 0.0f;
    uint32_t lfoLfsr     = 0xBEEF1234u;

    // Expression state
    float modWheelValue  = 0.0f;
    float aftertouchValue = 0.0f;

    // Voice allocation
    uint64_t noteOrderCounter = 0;

    // FDN delay ratios (cached per block)
    std::array<float, kVoices> delayRatios = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    // Crossfade tracking
    int previousKnot = -1;

    // Sample rate
    double sr = 44100.0;

    // Coupling inputs
    float extPitchMod  = 0.0f;  // semitones — routes to tangle depth perturbation
    float extFilterMod = 0.0f;  // Hz offset — AmpToFilter / FilterToFilter
    float extRingMod   = 0.0f;  // amplitude factor — AudioToRing
    float extDelayMod  = 0.0f;  // relative modulation of FDN delay base — AudioToFM

    // Cached filter cutoff for FilterToFilter coupling (set during renderBlock)
    float modFilterCutoffCache = 8000.0f;

    // Coupling output cache (updated per sample)
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    // Active voice count for XOmnibus display
    int activeCount = 0;

    //==========================================================================
    // LFO xorshift noise
    float lfoNoise() noexcept
    {
        lfoLfsr ^= lfoLfsr << 13;
        lfoLfsr ^= lfoLfsr >> 17;
        lfoLfsr ^= lfoLfsr << 5;
        return static_cast<float>(static_cast<int32_t>(lfoLfsr)) / 2147483648.0f;
    }

    // LFO waveform computation (5 shapes)
    float computeLFO(float phase, int shape, float& shValue) noexcept
    {
        switch (shape)
        {
            case 0: return fastSin(phase * 6.28318530f);
            case 1: return 2.0f * std::fabs(2.0f * phase - 1.0f) - 1.0f;
            case 2: return 1.0f - 2.0f * phase;
            case 3: return phase < 0.5f ? 1.0f : -1.0f;
            case 4: return shValue;
            default: return 0.0f;
        }
    }

    // LFO modulation routing (6 destinations)
    void applyLFOModulation(float lfoValue, float depth, int dest,
                            float& tangleDepth, float& dampening, float& pulseRate,
                            float& delayBase, float& filterCutoff, float& spread) noexcept
    {
        float mod = lfoValue * depth;
        switch (dest)
        {
            case 0: tangleDepth  = std::clamp(tangleDepth + mod * 0.5f, 0.0f, 1.0f); break;
            case 1: dampening    = std::clamp(dampening + mod * 0.5f, 0.0f, 1.0f); break;
            case 2: pulseRate    = std::max(0.01f, pulseRate * (1.0f + mod * 0.5f)); break;
            case 3: delayBase    = std::max(1.0f, delayBase * (1.0f + mod * 0.3f)); break;
            case 4: filterCutoff = std::clamp(filterCutoff * fastPow2(mod * 2.0f), 20.0f, 20000.0f); break;
            case 5: spread       = std::clamp(spread + mod * 0.5f, 0.0f, 1.0f); break;
        }
    }

    // Voice allocation — free first, then oldest released, then oldest active
    int allocateVoice() noexcept
    {
        for (int i = 0; i < kVoices; ++i)
            if (!voices[static_cast<size_t>(i)].isActive())
                return i;

        int bestIdx     = -1;
        uint64_t bestOrder = UINT64_MAX;
        for (int i = 0; i < kVoices; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            if (!v.held && v.noteOffOrder < bestOrder)
            {
                bestOrder = v.noteOffOrder;
                bestIdx   = i;
            }
        }
        if (bestIdx >= 0) return bestIdx;

        bestIdx    = 0;
        bestOrder  = voices[0].noteOnOrder;
        for (int i = 1; i < kVoices; ++i)
        {
            if (voices[static_cast<size_t>(i)].noteOnOrder < bestOrder)
            {
                bestOrder = voices[static_cast<size_t>(i)].noteOnOrder;
                bestIdx   = i;
            }
        }
        return bestIdx;
    }

    void handleNoteOn(int note, float velocity) noexcept
    {
        int idx = allocateVoice();
        auto& v = voices[static_cast<size_t>(idx)];
        v.noteOn(note, velocity, ++noteOrderCounter);
        v.env.setParams(params.attack, params.decay, params.sustain, params.release);

        filterEnvLevel    = 1.0f;
        filterEnvVelocity = velocity;
        float decaySec    = params.filterEnvDecay;
        filterEnvDecayRate = (decaySec > 0.0001f)
            ? 1.0f / (decaySec * static_cast<float>(sr))
            : 1.0f;
    }

    void handleNoteOff(int note) noexcept
    {
        for (int i = 0; i < kVoices; ++i)
        {
            auto& v = voices[static_cast<size_t>(i)];
            if (v.isActive() && v.held && v.midiNote == note)
                v.noteOff(++noteOrderCounter);
        }
    }
};

constexpr float XOverlapEngine::kPanPositions[6];

} // namespace xomnibus
