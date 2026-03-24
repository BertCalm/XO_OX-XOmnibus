#pragma once
#include "EntropyCooler.h"
#include "VoronoiShatter.h"
#include "QuantumSmear.h"
#include "AttractorDrive.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace xolokun {

//==============================================================================
// MathFXChain — 4 mathematical FX processors from the XOM-FX-CORE manifesto.
//
// Signal chain (fixed order):
//   1. Entropy Cooler  — Heat Eq + Shannon Entropy + Maxwell's Demon (filter)
//   2. Voronoi Shatter — Fibonacci Spiral + Voronoi + Tensegrity (granular)
//   3. Quantum Smear   — Schrödinger + Airy + Peano Curve (delay/reverb)
//   4. Attractor Drive — Lorenz + Logistic Map (distortion)
//
// Each stage bypasses at zero CPU when mix = 0.
// MIDI CC mapping: CC30 = Entropy Cooler, CC31 = Voronoi, CC32 = Quantum, CC33 = Attractor
//==============================================================================
class MathFXChain
{
public:
    MathFXChain() = default;

    void prepare (double sampleRate)
    {
        entropyCooler.prepare (sampleRate);
        voronoiShatter.prepare (sampleRate);
        quantumSmear.prepare (sampleRate);
        attractorDrive.prepare (sampleRate);
    }

    void reset()
    {
        entropyCooler.reset();
        voronoiShatter.reset();
        quantumSmear.reset();
        attractorDrive.reset();
    }

    //--------------------------------------------------------------------------
    // Process a stereo block in-place. Caller reads all params.
    void processBlock (float* L, float* R, int numSamples,
                       // Entropy Cooler
                       float ecStability, float ecCoolRate, float ecThreshold, float ecMix,
                       // Voronoi Shatter
                       float vsCrystallize, float vsTension, float vsGrainSize, float vsMix,
                       // Quantum Smear
                       float qsObservation, float qsFeedback, float qsDelayCenter, float qsMix,
                       // Attractor Drive
                       float adBifurcation, float adDriveBase, float adSpeed, float adMix)
    {
        entropyCooler.processBlock  (L, R, numSamples, ecStability, ecCoolRate, ecThreshold, ecMix);
        voronoiShatter.processBlock (L, R, numSamples, vsCrystallize, vsTension, vsGrainSize, vsMix);
        quantumSmear.processBlock   (L, R, numSamples, qsObservation, qsFeedback, qsDelayCenter, qsMix);
        attractorDrive.processBlock (L, R, numSamples, adBifurcation, adDriveBase, adSpeed, adMix);
    }

    //--------------------------------------------------------------------------
    // Parameter declarations for APVTS
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // 1. Entropy Cooler (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_ecStability", 1 }, "MathFX Entropy Stability",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_ecCoolRate", 1 }, "MathFX Entropy Cool Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_ecThreshold", 1 }, "MathFX Entropy Threshold",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_ecMix", 1 }, "MathFX Entropy Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 2. Voronoi Shatter (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_vsCrystallize", 1 }, "MathFX Voronoi Crystallize",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_vsTension", 1 }, "MathFX Voronoi Tension",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_vsGrainSize", 1 }, "MathFX Voronoi Grain Size",
            juce::NormalisableRange<float> (5.0f, 100.0f, 0.1f, 0.4f), 30.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_vsMix", 1 }, "MathFX Voronoi Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 3. Quantum Smear (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_qsObservation", 1 }, "MathFX Quantum Observation",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_qsFeedback", 1 }, "MathFX Quantum Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.4f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_qsDelayCenter", 1 }, "MathFX Quantum Delay Center",
            juce::NormalisableRange<float> (10.0f, 500.0f, 0.1f, 0.4f), 150.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_qsMix", 1 }, "MathFX Quantum Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 4. Attractor Drive (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_adBifurcation", 1 }, "MathFX Attractor Bifurcation",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_adDriveBase", 1 }, "MathFX Attractor Drive Base",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_adSpeed", 1 }, "MathFX Attractor Speed",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "mfx_adMix", 1 }, "MathFX Attractor Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    }

private:
    EntropyCooler  entropyCooler;
    VoronoiShatter voronoiShatter;
    QuantumSmear   quantumSmear;
    AttractorDrive attractorDrive;
};

} // namespace xolokun
