#pragma once

//==============================================================================
// XOutwitAdapter.h — XOceanus adapter for XOutwit (OUTWIT)
//
// 8-arm Wolfram cellular automaton synthesizer. Giant Pacific Octopus —
// eight independent voice-channels each running their own Wolfram rule (0-255).
// Distributed intelligence as synthesis. SOLVE macro hunts for sound via GA.
//
// Gallery code: OUTWIT | Accent: Chromatophore Amber #CC6600
// Parameter prefix: owit_ | Macros: M1=SOLVE, M2=SYNAPSE, M3=CHROMATOPHORE, M4=DEN
// Coupling output: 9 types supported
//
// XOceanus integration: DSP headers resolved via target_include_directories
// pointing to XOutwit/Source/ — see CMakeLists.txt.
//==============================================================================

#include "../../Core/SynthEngine.h"

// XOutwit DSP — included via target_include_directories in CMakeLists.txt
// (path: XOutwit/Source/DSP/)
#include "DSP/ArmChannel.h"
#include "DSP/AmpEnvelope.h"
#include "DSP/DenReverb.h"
#include "DSP/InkCloud.h"
#include "DSP/ParamSnapshot.h"
#include "DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>

namespace xoceanus {

class XOutwitEngine : public SynthEngine {
public:

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        silenceGate.prepare(sampleRate, maxBlockSize);
        for (auto& arm : arms)
            arm.prepare(sampleRate);
        ampEnv.prepare(sampleRate);
        denReverb.prepare(sampleRate);
        denReverb.clear();
        noteHeld = false;
        currentNote = -1;
        targetNote  = -1;
        glideFreq   = 0.0f;
        glideCoeff  = 1.0f;
        lfo1Phase = lfo2Phase = 0.0f;
        lfo1SHValue = lfo2SHValue = 0.0f;
        modWheelValue = aftertouchValue = 0.0f;
        extStepRateMod = extChromMod = extSynapseMod = extFilterMod = extPitchMod = 0.0f;
        extAmpMod = 1.0f;
        lastSampleL = lastSampleR = 0.0f;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& arm : arms)
            arm.prepare(sr);
        ampEnv.prepare(sr);
        denReverb.prepare(sr);
        denReverb.clear();
        lfo1Phase = lfo2Phase = 0.0f;
        lfo1SHValue = lfo2SHValue = 0.0f;
        noteHeld = false;
        currentNote = -1;
        targetNote  = -1;
        glideFreq   = 0.0f;
        glideCoeff  = 1.0f;
        extStepRateMod = extChromMod = extSynapseMod = extFilterMod = extPitchMod = 0.0f;
        extAmpMod = 1.0f;
        lastSampleL = lastSampleR = 0.0f;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    // renderBlock — ACCUMULATES into buf (+=), does not clear
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi, int ns) override
    {
        juce::ScopedNoDenormals noDenormals;

        if (buf.getNumChannels() < 2) return;

        // 1. Cache parameters once per block
        snap.update();

        // 2. LFO computation (once per block at block midpoint)
        float invSr = 1.0f / static_cast<float>(sr);
        float lfo1Val = computeLFO(lfo1Phase, snap.lfo1Shape, lfo1SHValue);
        float lfo2Val = computeLFO(lfo2Phase, snap.lfo2Shape, lfo2SHValue);
        lfo1Phase += snap.lfo1Rate * invSr * static_cast<float>(ns);
        lfo2Phase += snap.lfo2Rate * invSr * static_cast<float>(ns);
        if (lfo1Phase >= 1.0f) { lfo1Phase -= 1.0f; if (snap.lfo1Shape == 4) lfo1SHValue = lfoNoise(); }
        if (lfo2Phase >= 1.0f) { lfo2Phase -= 1.0f; if (snap.lfo2Shape == 4) lfo2SHValue = lfoNoise(); }

        // 3. Working copies — LFO + macro + expression modulation
        float modStepRate    = snap.stepRate * (1.0f + snap.huntRate); // huntRate scales CA search speed
        float modChromAmount = snap.chromAmount;
        float modSynapse     = snap.synapse;
        float modDenSize     = snap.denSize;
        float modDenDecay    = snap.denDecay;
        float modDenMix      = snap.denMix;
        float modArmLevelScale = 1.0f;

        // Apply coupling ext mods
        modStepRate    += extStepRateMod;
        modChromAmount  = clamp(modChromAmount + extChromMod,  0.0f, 1.0f);
        modSynapse      = clamp(modSynapse + extSynapseMod,    0.0f, 1.0f);
        modArmLevelScale *= extAmpMod;

        // 4. Apply LFOs
        auto applyLfo = [&](float lfoVal, float depth, int dest) {
            float mod = lfoVal * depth;
            switch (dest) {
                case 0: modStepRate    = juce::jmax(0.01f, modStepRate * (1.0f + mod * 0.5f)); break;
                case 1: extFilterMod += mod * 4000.0f; break; // LFO→FilterCutoff: ±4kHz sweep
                case 2: modChromAmount = clamp(modChromAmount + mod * 0.5f, 0.0f, 1.0f); break;
                case 3: modArmLevelScale = clamp(modArmLevelScale + mod * 0.5f, 0.0f, 1.5f); break;
                default: break;
            }
        };
        if (snap.lfo1Depth > 0.001f) applyLfo(lfo1Val, snap.lfo1Depth, snap.lfo1Dest);
        if (snap.lfo2Depth > 0.001f) applyLfo(lfo2Val, snap.lfo2Depth, snap.lfo2Dest);

        // 5. Apply macros
        {
            float ms = snap.macroSolve;
            float modSolveAmt = clamp(snap.solve + ms, 0.0f, 1.0f);

            // D004 FIX: SOLVE macro uses modSolveAmt to steer DSP toward target DNA.
            // Each target dimension biases a real DSP parameter; strength scales with
            // modSolveAmt so SOLVE=0 is a true no-op and SOLVE=1 is full target steering.
            if (modSolveAmt > 0.001f)
            {
                // targetMovement → step rate (more movement = faster CA stepping)
                // Bias up to ±16 Hz over the base step rate
                float movementBias = (snap.targetMovement - 0.5f) * 2.0f * 16.0f;
                modStepRate = juce::jmax(0.01f, modStepRate + movementBias * modSolveAmt);

                // targetBrightness → chromatophore amount (brighter = more filter modulation)
                float brightnessBias = (snap.targetBrightness - 0.5f) * 2.0f * 0.5f;
                modChromAmount = clamp(modChromAmount + brightnessBias * modSolveAmt, 0.0f, 1.0f);

                // targetSpace → den reverb mix (more space = more reverb)
                float spaceBias = (snap.targetSpace - 0.5f) * 2.0f * 0.4f;
                modDenMix   = clamp(modDenMix   + spaceBias * modSolveAmt, 0.0f, 1.0f);
                modDenSize  = clamp(modDenSize  + spaceBias * modSolveAmt * 0.7f, 0.0f, 1.0f);
                modDenDecay = clamp(modDenDecay + spaceBias * modSolveAmt * 0.5f, 0.0f, 1.0f);

                // targetAggression → arm level scale (aggression = louder, more present arms)
                float aggressionBias = (snap.targetAggression - 0.5f) * 2.0f * 0.4f;
                modArmLevelScale = clamp(modArmLevelScale + aggressionBias * modSolveAmt, 0.1f, 2.0f);

                // targetSynapse (warmth proxy) → synapse coupling
                // Warmth = arms coupled more closely, creating tighter rhythmic density
                float warmthBias = (snap.targetWarmth - 0.5f) * 2.0f * 0.4f;
                modSynapse = clamp(modSynapse + warmthBias * modSolveAmt, 0.0f, 1.0f);

                // targetDensity → CA rule bias per arm (stored for use in arm loop below)
                // High density target nudges rules toward busier patterns; stored in member.
                solveDensityBias = (snap.targetDensity - 0.5f) * 2.0f; // [-1, +1]
                solveAmt         = modSolveAmt;
            }
            else
            {
                solveDensityBias = 0.0f;
                solveAmt         = 0.0f;
            }

            modSynapse      = clamp(modSynapse      + snap.macroSynapse * 0.6f,        0.0f, 1.0f);
            modChromAmount  = clamp(modChromAmount  + snap.macroChromatophore * 0.7f,  0.0f, 1.0f);
            modDenSize      = clamp(modDenSize      + snap.macroDen * 0.6f,            0.0f, 1.0f);
            modDenDecay     = clamp(modDenDecay     + snap.macroDen * 0.5f,            0.0f, 1.0f);
            modDenMix       = clamp(modDenMix       + snap.macroDen * 0.4f,            0.0f, 1.0f);
        }

        // 6. Apply expression (mod wheel → SYNAPSE, aftertouch → ChromAmount)
        {
            float mwMod = modWheelValue * 0.4f;
            modSynapse     = clamp(modSynapse + mwMod, 0.0f, 1.0f);

            float atMod = aftertouchValue * 0.3f;
            modChromAmount = clamp(modChromAmount + atMod, 0.0f, 1.0f);
        }

        // 7. Update DSP module params
        ampEnv.setParams(snap.ampAttack, snap.ampDecay, snap.ampSustain, snap.ampRelease);

        // extFilterMod adds to each arm's base filter cutoff
        for (int n = 0; n < 8; ++n)
        {
            auto i = static_cast<size_t>(n);
            float armFilterHz = clamp(snap.armFilter[i] + extFilterMod, 20.0f, 20000.0f);
            // extPitchMod: semitone offset added to arm pitch
            int   armPitchSt  = snap.armPitch[i] + static_cast<int>(std::round(extPitchMod));

            // D004 FIX: SOLVE/targetDensity biases the CA rule toward denser or sparser
            // patterns. solveDensityBias in [-1,+1]: positive → rule nudged toward 255
            // (all-on), negative → rule nudged toward 0 (all-off).
            // Max rule offset is ±64 steps, scaled by solveAmt.
            int effectiveRule = snap.armRule[i];
            if (solveAmt > 0.001f)
            {
                int ruleOffset = static_cast<int>(solveDensityBias * 64.0f * solveAmt);
                effectiveRule  = clamp(effectiveRule + ruleOffset, 0, 255);
            }

            arms[i].setParams(effectiveRule, snap.armLength[i],
                               snap.armLevel[i] * modArmLevelScale,
                               armPitchSt, armFilterHz,
                               snap.armWave[i], snap.armPan[i]);
            arms[i].setStepRate(modStepRate);

            // Seance P1: pass glide rate to each arm (1-coeff = per-sample smoothing amount)
            float rate = (snap.voiceMode == 1 && snap.glide > 0.001f)
                         ? (1.0f - glideCoeff) : 1.0f;
            arms[i].setGlideRate(rate);
        }

        denReverb.setParams(modDenSize, modDenDecay, sr);
        inkCloud.prepare(sr, snap.inkDecay);

        auto* outL = buf.getWritePointer(0);
        auto* outR = buf.getWritePointer(1);

        // SilenceGate: pre-pass scan for note-on to wake gate before bypass check
        for (const auto& meta : midi)
            if (meta.getMessage().isNoteOn())
                silenceGate.wake();

        if (silenceGate.isBypassed() && midi.isEmpty()) { buf.clear(); return; }

        // 8. Process MIDI and audio interleaved
        int samplePos = 0;
        for (const auto metadata : midi)
        {
            const int eventPos = metadata.samplePosition;

            for (; samplePos < eventPos; ++samplePos)
            {
                float sL, sR;
                renderOneSample(modChromAmount, modSynapse, modDenMix, sL, sR);
                // ACCUMULATE
                outL[samplePos] += sL;
                outR[samplePos] += sR;
                lastSampleL = sL;
                lastSampleR = sR;
            }

            handleMidiEvent(metadata.getMessage());
        }

        // Render remaining samples after last MIDI event
        for (; samplePos < ns; ++samplePos)
        {
            float sL, sR;
            renderOneSample(modChromAmount, modSynapse, modDenMix, sL, sR);
            outL[samplePos] += sL;
            outR[samplePos] += sR;
            lastSampleL = sL;
            lastSampleR = sR;
        }

        activeVoiceCount_.store(noteHeld ? 1 : 0, std::memory_order_relaxed);

        silenceGate.analyzeBlock(buf.getReadPointer(0), buf.getReadPointer(1), ns);
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling(int ch, int) const override
    {
        return ch == 0 ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput(CouplingType t, float amount,
                            const float* buf, int /*ns*/) override
    {
        switch (t)
        {
            case CouplingType::AudioToFM:
                // Engine A audio → step rate modulation
                extStepRateMod = (buf ? buf[0] : 0.f) * amount * 20.0f; // ±20Hz step rate mod
                break;
            case CouplingType::AmpToFilter:
                // Engine A amplitude → arm filter cutoff (opens filter)
                extFilterMod = amount * 8000.0f; // 0 → +8kHz
                break;
            case CouplingType::EnvToMorph:
                // Engine A envelope → chromatophore depth
                extChromMod = amount * 0.7f;
                break;
            case CouplingType::LFOToPitch:
                extPitchMod = (buf ? buf[0] : 0.f) * amount * 12.0f; // ±12 semitones
                break;
            case CouplingType::RhythmToBlend:
                // Engine A rhythm pattern → synapse coupling
                extSynapseMod = amount * 0.5f;
                break;
            case CouplingType::AmpToChoke:
                // Engine A amplitude chokes arm levels
                extAmpMod = std::max(0.0f, 1.0f - amount);
                break;
            case CouplingType::AudioToRing:
                // Ring modulate — apply as amplitude scaling
                extAmpMod *= (buf ? std::abs(buf[0]) : 1.0f) * amount + (1.0f - amount);
                break;
            case CouplingType::FilterToFilter:
                // External filter → arm filter cutoff offset
                extFilterMod += amount * 4000.0f;
                break;
            case CouplingType::PitchToPitch:
                extPitchMod += amount * 7.0f; // harmony offset
                break;
            default: break;
        }
    }

    //==========================================================================
    // W07 fix: addParameters — called by XOceanusProcessor::createParameterLayout()
    // to register all owit_ params in the shared APVTS.
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // Per-arm parameters (x8)
        for (int n = 0; n < 8; ++n)
        {
            auto prefix = "owit_arm" + juce::String(n);

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                prefix + "Rule", "Arm " + juce::String(n) + " Rule", 0, 255,
                std::array<int,8>{110, 30, 90, 184, 60, 45, 150, 105}[static_cast<size_t>(n)]));

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                prefix + "Length", "Arm " + juce::String(n) + " Length", 4, 64, 16));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "Level", "Arm " + juce::String(n) + " Level",
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                prefix + "Pitch", "Arm " + juce::String(n) + " Pitch", -24, 24, 0));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "Filter", "Arm " + juce::String(n) + " Filter",
                juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 4000.0f));

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                prefix + "Wave", "Arm " + juce::String(n) + " Wave",
                juce::StringArray{"Saw", "Pulse", "Sine"}, 0));

            auto defaultPan = std::array<float,8>{-0.8f, -0.5f, -0.2f, 0.1f, 0.3f, 0.5f, 0.7f, 0.9f}[static_cast<size_t>(n)];
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "Pan", "Arm " + juce::String(n) + " Pan",
                juce::NormalisableRange<float>(-1.0f, 1.0f), defaultPan));
        }

        // Global: Step/Clock
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_stepRate", "Step Rate",
            juce::NormalisableRange<float>(0.01f, 40.0f, 0.0f, 0.4f), 4.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>("owit_stepSync", "Step Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_stepDiv", "Step Division",
            juce::StringArray{"1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4","1/2","1/1","2/1"}, 4));

        // Global: Coupling/Intelligence
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_synapse", "Synapse",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_chromAmount", "Chromatophore",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_solve", "SOLVE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_huntRate", "Hunt Rate",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // SOLVE target DNA
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetBrightness", "Target Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetWarmth", "Target Warmth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetMovement", "Target Movement",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetDensity", "Target Density",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetSpace", "Target Space",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetAggression", "Target Aggression",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Ink Cloud
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_inkCloud", "Ink Cloud",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_inkDecay", "Ink Decay",
            juce::NormalisableRange<float>(0.01f, 0.5f, 0.0f, 0.5f), 0.08f));

        // Voice
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_triggerThresh", "Trigger Threshold",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_masterLevel", "Master Level",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

        // Amp envelope
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampAttack", "Amp Attack",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.01f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampDecay", "Amp Decay",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampSustain", "Amp Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampRelease", "Amp Release",
            juce::NormalisableRange<float>(0.001f, 4.0f, 0.0f, 0.3f), 0.3f));

        // Filter
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_filterRes", "Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_filterType", "Filter Type",
            juce::StringArray{"LP", "BP", "HP"}, 0));

        // Den Reverb
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_denSize", "Den Size",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_denDecay", "Den Decay",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_denMix", "Den Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // LFOs
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo1Rate", "LFO 1 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.35f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo1Depth", "LFO 1 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo1Shape", "LFO 1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo1Dest", "LFO 1 Dest",
            juce::StringArray{"StepRate", "FilterCutoff", "ChromAmount", "ArmLevels"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo2Rate", "LFO 2 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.35f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo2Depth", "LFO 2 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo2Shape", "LFO 2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo2Dest", "LFO 2 Dest",
            juce::StringArray{"StepRate", "FilterCutoff", "ChromAmount", "ArmLevels"}, 1));

        // Voice mode
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_voiceMode", "Voice Mode",
            juce::StringArray{"Poly", "Mono"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_glide", "Glide",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f));

        // Macros
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroSolve", "M1 SOLVE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroSynapse", "M2 SYNAPSE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroChromatophore", "M3 CHROMATOPHORE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroDen", "M4 DEN",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    //==========================================================================
    // Parameter layout — exact copy of standalone createParameterLayout()
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Per-arm parameters (x8)
        for (int n = 0; n < 8; ++n)
        {
            auto prefix = "owit_arm" + juce::String(n);

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                prefix + "Rule", "Arm " + juce::String(n) + " Rule", 0, 255,
                std::array<int,8>{110, 30, 90, 184, 60, 45, 150, 105}[static_cast<size_t>(n)]));

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                prefix + "Length", "Arm " + juce::String(n) + " Length", 4, 64, 16));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "Level", "Arm " + juce::String(n) + " Level",
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                prefix + "Pitch", "Arm " + juce::String(n) + " Pitch", -24, 24, 0));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "Filter", "Arm " + juce::String(n) + " Filter",
                juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 4000.0f));

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                prefix + "Wave", "Arm " + juce::String(n) + " Wave",
                juce::StringArray{"Saw", "Pulse", "Sine"}, 0));

            auto defaultPan = std::array<float,8>{-0.8f, -0.5f, -0.2f, 0.1f, 0.3f, 0.5f, 0.7f, 0.9f}[static_cast<size_t>(n)];
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                prefix + "Pan", "Arm " + juce::String(n) + " Pan",
                juce::NormalisableRange<float>(-1.0f, 1.0f), defaultPan));
        }

        // Global: Step/Clock
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_stepRate", "Step Rate",
            juce::NormalisableRange<float>(0.01f, 40.0f, 0.0f, 0.4f), 4.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>("owit_stepSync", "Step Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_stepDiv", "Step Division",
            juce::StringArray{"1/32","1/16T","1/16","1/8T","1/8","1/4T","1/4","1/2","1/1","2/1"}, 4));

        // Global: Coupling/Intelligence
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_synapse", "Synapse",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_chromAmount", "Chromatophore",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_solve", "SOLVE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_huntRate", "Hunt Rate",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // SOLVE target DNA
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetBrightness", "Target Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetWarmth", "Target Warmth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetMovement", "Target Movement",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetDensity", "Target Density",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetSpace", "Target Space",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_targetAggression", "Target Aggression",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Ink Cloud
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_inkCloud", "Ink Cloud",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_inkDecay", "Ink Decay",
            juce::NormalisableRange<float>(0.01f, 0.5f, 0.0f, 0.5f), 0.08f));

        // Voice
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_triggerThresh", "Trigger Threshold",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_masterLevel", "Master Level",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

        // Amp envelope
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampAttack", "Amp Attack",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.01f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampDecay", "Amp Decay",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampSustain", "Amp Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_ampRelease", "Amp Release",
            juce::NormalisableRange<float>(0.001f, 4.0f, 0.0f, 0.3f), 0.3f));

        // Filter
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_filterRes", "Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_filterType", "Filter Type",
            juce::StringArray{"LP", "BP", "HP"}, 0));

        // Den Reverb
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_denSize", "Den Size",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_denDecay", "Den Decay",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_denMix", "Den Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // LFOs
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo1Rate", "LFO 1 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.35f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo1Depth", "LFO 1 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo1Shape", "LFO 1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo1Dest", "LFO 1 Dest",
            juce::StringArray{"StepRate", "FilterCutoff", "ChromAmount", "ArmLevels"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo2Rate", "LFO 2 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.35f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_lfo2Depth", "LFO 2 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo2Shape", "LFO 2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_lfo2Dest", "LFO 2 Dest",
            juce::StringArray{"StepRate", "FilterCutoff", "ChromAmount", "ArmLevels"}, 1));

        // Voice mode
        params.push_back(std::make_unique<juce::AudioParameterChoice>("owit_voiceMode", "Voice Mode",
            juce::StringArray{"Poly", "Mono"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_glide", "Glide",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f));

        // Macros
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroSolve", "M1 SOLVE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroSynapse", "M2 SYNAPSE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroChromatophore", "M3 CHROMATOPHORE",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("owit_macroDen", "M4 DEN",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        return { params.begin(), params.end() };
    }

    //==========================================================================
    // attachParameters — store pointer and delegate to snap
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        apvts_ptr = &apvts;
        snap.attachParameters(apvts);
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId()    const override { return "Outwit"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFCC6600); } // Chromatophore Amber
    int getMaxVoices()            const override { return 1; }
    int getActiveVoiceCount()     const override { return activeVoiceCount_.load(std::memory_order_relaxed); }

private:

    //==========================================================================
    // DSP state
    //==========================================================================

    SilenceGate silenceGate;
    std::array<xoutwit::ArmChannel, 8> arms;
    xoutwit::AmpEnvelope ampEnv;
    xoutwit::DenReverb   denReverb;
    xoutwit::InkCloud    inkCloud;
    xoutwit::ParamSnapshot snap;

    double sr = 48000.0;
    bool   noteHeld = false;
    int    currentNote = -1;
    int    targetNote  = -1;   // Seance P1: glide target note
    float  glideFreq   = 0.0f; // Seance P1: current glide frequency (Hz)
    float  glideCoeff  = 1.0f; // Seance P1: smoothing coefficient for portamento
    float  currentVelocity = 0.0f;

    float lfo1Phase = 0.0f, lfo2Phase = 0.0f;
    float lfo1SHValue = 0.0f, lfo2SHValue = 0.0f;
    uint32_t lfoLfsr = 0xCAFE5A3Bu;

    float modWheelValue = 0.0f;
    float aftertouchValue = 0.0f;

    float lastSampleL = 0.0f, lastSampleR = 0.0f;
    // DSP Fix Wave 2B: Den reverb stereo decorrelation state
    float denStereoState = 0.0f;

    // Coupling ext mods
    float extStepRateMod = 0.0f;   // AudioToFM → step rate
    float extChromMod    = 0.0f;   // EnvToMorph → chrom amount
    float extSynapseMod  = 0.0f;   // RhythmToBlend → synapse
    float extFilterMod   = 0.0f;   // AmpToFilter → arm filter
    float extPitchMod    = 0.0f;   // LFOToPitch → pitch offset (semitones)
    float extAmpMod      = 1.0f;   // AmpToChoke → arm level multiplier (1.0 = no choke)

    // SOLVE macro state — computed in macro block, consumed in arm loop (same block)
    float solveDensityBias = 0.0f; // [-1,+1] targetDensity bias for CA rule offset
    float solveAmt         = 0.0f; // [0,1]   effective SOLVE strength this block

    // activeCount promoted to base class activeVoiceCount_
    juce::AudioProcessorValueTreeState* apvts_ptr = nullptr;

    //==========================================================================
    // Helpers
    //==========================================================================

    float lfoNoise() noexcept
    {
        lfoLfsr ^= lfoLfsr << 13;
        lfoLfsr ^= lfoLfsr >> 17;
        lfoLfsr ^= lfoLfsr << 5;
        return static_cast<float>((int32_t)lfoLfsr) * 4.656612e-10f;
    }

    float computeLFO(float phase, int shape, float& shValue) noexcept
    {
        switch (shape)
        {
            case 0: return std::sin(phase * 6.28318530f);                         // Sine
            case 1: return (phase < 0.5f) ? (4.0f * phase - 1.0f)                // Triangle
                                          : (3.0f - 4.0f * phase);
            case 2: return 2.0f * phase - 1.0f;                                   // Saw
            case 3: return phase < 0.5f ? 1.0f : -1.0f;                          // Square
            case 4: return shValue;                                                // S&H (held until phase wraps)
            default: return 0.0f;
        }
    }

    void handleMidiEvent(const juce::MidiMessage& msg)
    {
        if (msg.isNoteOn())
        {
            bool wasHeld = noteHeld;
            bool monoLegato = (snap.voiceMode == 1) && wasHeld && (snap.glide > 0.001f);

            currentNote     = msg.getNoteNumber();
            targetNote      = currentNote;
            currentVelocity = msg.getFloatVelocity();

            // Seance P1: voiceMode/glide wiring (D004 fix)
            // Mono legato: do NOT re-trigger CA/env — just glide to new pitch
            if (monoLegato)
            {
                // Update arm pitch targets without resetting CA or envelope
                for (auto& arm : arms)
                    arm.setGlideTarget(currentNote, currentVelocity);
            }
            else
            {
                // Normal trigger (Poly mode, or first note in Mono, or Mono with glide=0)
                for (auto& arm : arms)
                    arm.noteOn(currentNote, currentVelocity);

                ampEnv.noteOn();
                inkCloud.trigger(currentVelocity, snap.inkCloud);
                glideFreq = xoutwit::midiToFreq(currentNote);
            }

            // Compute glide coefficient from glide time (Seance P1)
            // glide [0,1] maps to [0ms, 500ms] portamento time
            if (snap.glide > 0.001f && snap.voiceMode == 1)
            {
                float glideTimeSec = snap.glide * 0.5f; // 0-500ms
                glideCoeff = xoutwit::fastExp(-1.0f / (glideTimeSec * static_cast<float>(sr)));
            }
            else
            {
                glideCoeff = 0.0f; // instant — no portamento
            }

            noteHeld = true;
        }
        else if (msg.isNoteOff())
        {
            if (msg.getNoteNumber() == currentNote)
            {
                for (auto& arm : arms)
                    arm.noteOff();

                ampEnv.noteOff();
                noteHeld = false;
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            for (auto& arm : arms)
                arm.noteOff();

            ampEnv.noteOff();
            noteHeld = false;
        }
        else if (msg.isController() && msg.getControllerNumber() == 1)
            modWheelValue = msg.getControllerValue() / 127.0f;
        else if (msg.isChannelPressure())
            aftertouchValue = msg.getChannelPressureValue() / 127.0f;
        // DSP Fix Wave 2B: Wire pitch wheel to arm pitch (was unhandled)
        else if (msg.isPitchWheel())
        {
            float bend = (msg.getPitchWheelValue() - 8192) / 8192.0f; // -1..+1
            extPitchMod = bend * 2.0f; // ±2 semitones pitch bend (SET, not accumulate)
        }
    }

    // Render one sample — shared by MIDI-interleaved and tail loops
    void renderOneSample(float modChromAmount, float modSynapse, float modDenMix,
                         float& outL, float& outR) noexcept
    {
        // Process each arm
        std::array<float, 8> armSamples{};
        for (int n = 0; n < 8; ++n)
        {
            armSamples[static_cast<size_t>(n)] = arms[static_cast<size_t>(n)].processSample(
                0.0f, modChromAmount, snap.filterRes, snap.filterType, snap.triggerThresh);
        }

        // SYNAPSE coupling: arm N drives arm N+1
        if (modSynapse > 0.001f)
        {
            for (int n = 0; n < 8; ++n)
            {
                auto src = static_cast<size_t>(n);
                auto dst = static_cast<size_t>((n + 1) % 8);
                if (arms[src].justStepped)
                    arms[dst].applySynapse(arms[src].getDensity(), modSynapse);
            }
        }

        // Mix 8 arms to stereo (equal-power panning)
        float mixL = 0.0f, mixR = 0.0f;
        for (int n = 0; n < 8; ++n)
        {
            auto i = static_cast<size_t>(n);
            float s = armSamples[i];
            float p = snap.armPan[i];
            float gainL = std::sqrt(0.5f * (1.0f - p));
            float gainR = std::sqrt(0.5f * (1.0f + p));
            mixL += s * gainL;
            mixR += s * gainR;
        }

        // Amp envelope
        float env = ampEnv.process();
        mixL *= env;
        mixR *= env;

        // Ink cloud (mono, mixed equally)
        float ink = inkCloud.process();
        mixL += ink;
        mixR += ink;

        // Master level
        mixL *= snap.masterLevel;
        mixR *= snap.masterLevel;

        // Soft limiter
        mixL = xoutwit::fastTanh(mixL);
        mixR = xoutwit::fastTanh(mixR);

        // DSP Fix Wave 2B: Stereo-widened Den reverb — process L/R independently
        // to prevent mono collapse flagged in the 7.9 seance. We feed slightly
        // different signals (mid/side decorrelation) into the mono reverb and
        // reconstruct a wider stereo image from the result.
        float dry     = 1.0f - modDenMix;
        float wetGain = modDenMix * 0.7f;
        float wetMono = denReverb.process((mixL + mixR) * 0.5f);
        // Pseudo-stereo: allpass decorrelation using a tiny delay difference
        denStereoState = flushDenormal(denStereoState * 0.93f + wetMono * 0.07f);
        float wetL = wetMono;
        float wetR = denStereoState; // slightly decorrelated from left
        outL = mixL * dry + wetL * wetGain;
        outR = mixR * dry + wetR * wetGain;
    }
};

} // namespace xoceanus
