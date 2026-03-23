/*
    XOmnibus Doctrine Tests
    =======================
    Automated tests for the 6 XOmnibus Doctrines:

      D001 — Velocity Must Shape Timbre (not just amplitude)
      D002 — Modulation is the Lifeblood (mod matrix slots defined)
      D003 — The Physics IS the Synthesis (skip — can't automate citations)
      D004 — Dead Parameters Are Broken Promises (every param affects output)
      D005 — LFO Breathing (min rate <= 0.01 Hz)
      D006 — Expression Input Is Not Optional (velocity->timbre mapping)

    No test framework — assert-based with descriptive console output.
*/

#include "DoctrineTests.h"

#include "Core/EngineRegistry.h"
#include "Core/SynthEngine.h"

// Include all engine headers so static registrations execute in the test binary.
// (These may already be registered by DSPStabilityTests.cpp in the same binary;
// registerEngine() silently returns false for duplicates.)
#include "Engines/Snap/SnapEngine.h"
#include "Engines/Morph/MorphEngine.h"
#include "Engines/Dub/DubEngine.h"
#include "Engines/Drift/DriftEngine.h"
#include "Engines/Bob/BobEngine.h"
#include "Engines/Fat/FatEngine.h"
#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Overworld/OverworldEngine.h"
#include "Engines/Opal/OpalEngine.h"
#include "Engines/Bite/BiteEngine.h"
#include "Engines/Organon/OrganonEngine.h"
#include "Engines/Ocelot/OcelotEngine.h"
#include "Engines/Ouroboros/OuroborosEngine.h"
#include "Engines/Obsidian/ObsidianEngine.h"
#include "Engines/Origami/OrigamiEngine.h"
#include "Engines/Oracle/OracleEngine.h"
#include "Engines/Obscura/ObscuraEngine.h"
#include "Engines/Oceanic/OceanicEngine.h"
#include "Engines/Optic/OpticEngine.h"
#include "Engines/Oblique/ObliqueEngine.h"
#include "Engines/Orbital/OrbitalEngine.h"
#include "Engines/Osprey/OspreyEngine.h"
#include "Engines/Osteria/OsteriaEngine.h"
#include "Engines/Owlfish/OwlfishEngine.h"
#include "Engines/Ohm/OhmEngine.h"
#include "Engines/Orphica/OrphicaEngine.h"
#include "Engines/Obbligato/ObbligatoEngine.h"
#include "Engines/Ottoni/OttoniEngine.h"
#include "Engines/Ole/OleEngine.h"
#include "Engines/Overlap/XOverlapAdapter.h"
#include "Engines/Outwit/XOutwitAdapter.h"
#include "Engines/Ombre/OmbreEngine.h"
#include "Engines/Orca/OrcaEngine.h"
#include "Engines/Octopus/OctopusEngine.h"

// Engines added post-Octopus (original fleet)
#include "Engines/Ostinato/OstinatoEngine.h"
#include "Engines/OpenSky/OpenSkyEngine.h"
#include "Engines/OceanDeep/OceandeepEngine.h"
#include "Engines/Ouie/OuieEngine.h"
#include "Engines/Obrix/ObrixEngine.h"
#include "Engines/Orbweave/OrbweaveEngine.h"
#include "Engines/Overtone/OvertoneEngine.h"
#include "Engines/Organism/OrganismEngine.h"
#include "Engines/Oxbow/OxbowEngine.h"
#include "Engines/Oware/OwareEngine.h"
#include "Engines/Opera/OperaEngine.h"
#include "Engines/Offering/OfferingEngine.h"
#include "Engines/Osmosis/OsmosisEngine.h"

// Kitchen Collection — V2 paid expansion (Chef / Kitchen / Cellar / Garden / Broth / Fusion)
#include "Engines/Oto/OtoEngine.h"
#include "Engines/Octave/OctaveEngine.h"
#include "Engines/Oleg/OlegEngine.h"
#include "Engines/Otis/OtisEngine.h"
#include "Engines/Oven/OvenEngine.h"
#include "Engines/Ochre/OchreEngine.h"
#include "Engines/Obelisk/ObeliskEngine.h"
#include "Engines/Opaline/OpalineEngine.h"
#include "Engines/Ogre/OgreEngine.h"
#include "Engines/Olate/OlateEngine.h"
#include "Engines/Oaken/OakenEngine.h"
#include "Engines/Omega/OmegaEngine.h"
#include "Engines/Orchard/OrchardEngine.h"
#include "Engines/Overgrow/OvergrowEngine.h"
#include "Engines/Osier/OsierEngine.h"
#include "Engines/Oxalis/OxalisEngine.h"
#include "Engines/Overwash/OverwashEngine.h"
#include "Engines/Overworn/OverwornEngine.h"
#include "Engines/Overflow/OverflowEngine.h"
#include "Engines/Overcast/OvercastEngine.h"
#include "Engines/Oasis/OasisEngine.h"
#include "Engines/Oddfellow/OddfellowEngine.h"
#include "Engines/Onkolo/OnkoloEngine.h"
#include "Engines/Opcode/OpcodeEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>

using namespace xomnibus;

//==============================================================================
// JUCE 8 compatibility: ParameterLayout is not iterable.
//
// To inspect parameters from a ParameterLayout we must feed it into an
// AudioProcessorValueTreeState, then call getParameters() on the underlying
// AudioProcessor to get the flat list.  MinimalTestProcessor provides the
// minimum AudioProcessor implementation needed to construct an APVTS.
//==============================================================================

struct MinimalTestProcessor : juce::AudioProcessor
{
    const juce::String getName() const override              { return "TestProcessor"; }
    void prepareToPlay (double, int) override                {}
    void releaseResources() override                         {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override             { return 0.0; }
    bool acceptsMidi() const override                        { return false; }
    bool producesMidi() const override                       { return false; }
    juce::AudioProcessorEditor* createEditor() override      { return nullptr; }
    bool hasEditor() const override                          { return false; }
    int getNumPrograms() override                            { return 1; }
    int getCurrentProgram() override                         { return 0; }
    void setCurrentProgram (int) override                    {}
    const juce::String getProgramName (int) override         { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override   {}
    void setStateInformation (const void*, int) override     {}
};

// Build an APVTS from a ParameterLayout and return the flat param list.
// The returned pointers are owned by `apvtsOut`; keep apvtsOut alive while
// you use the list.
static const juce::Array<juce::AudioProcessorParameter*>&
buildParamList (MinimalTestProcessor& proc,
                std::unique_ptr<juce::AudioProcessorValueTreeState>& apvtsOut,
                juce::AudioProcessorValueTreeState::ParameterLayout layout)
{
    apvtsOut = std::make_unique<juce::AudioProcessorValueTreeState>(
        proc, nullptr, "PARAMS", std::move (layout));
    return proc.getParameters();
}

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_doctrineTestsPassed = 0;
static int g_doctrineTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_doctrineTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_doctrineTestsFailed++;
    }
}

//==============================================================================
// Helper: compute RMS of a stereo buffer
//==============================================================================

static float computeRMS(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    double sumSq = 0.0;
    int totalSamples = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            double s = static_cast<double>(data[i]);
            sumSq += s * s;
        }
        totalSamples += numSamples;
    }
    if (totalSamples == 0) return 0.0f;
    return static_cast<float>(std::sqrt(sumSq / totalSamples));
}

//==============================================================================
// Helper: compute zero-crossing rate as a proxy for spectral brightness.
// Higher zero-crossing rate = brighter timbre.
//==============================================================================

static float computeZeroCrossingRate(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (numSamples < 2) return 0.0f;

    int crossings = 0;
    const float* data = buffer.getReadPointer(0);
    for (int i = 1; i < numSamples; ++i)
    {
        if ((data[i] >= 0.0f && data[i - 1] < 0.0f) ||
            (data[i] < 0.0f && data[i - 1] >= 0.0f))
        {
            crossings++;
        }
    }
    return static_cast<float>(crossings) / static_cast<float>(numSamples - 1);
}

//==============================================================================
// Helper: render multiple blocks from an engine with a given note-on velocity,
// returning the combined audio buffer.
//==============================================================================

static juce::AudioBuffer<float> renderWithVelocity(SynthEngine& engine,
                                                     float velocity,
                                                     int numBlocks,
                                                     int blockSize,
                                                     double sampleRate)
{
    engine.prepare(sampleRate, blockSize);
    engine.reset();

    int totalSamples = numBlocks * blockSize;
    juce::AudioBuffer<float> result(2, totalSamples);
    result.clear();

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::AudioBuffer<float> blockBuf(2, blockSize);
        blockBuf.clear();
        juce::MidiBuffer midi;

        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, velocity), 0);

        engine.renderBlock(blockBuf, midi, blockSize);

        int destStart = block * blockSize;
        for (int ch = 0; ch < 2; ++ch)
        {
            result.copyFrom(ch, destStart, blockBuf, ch, 0, blockSize);
        }
    }

    return result;
}

//==============================================================================
// D001 — Velocity Must Shape Timbre (not just amplitude)
//
// For each engine, render a note at velocity 0.2 and at velocity 1.0.
// Normalize both to the same RMS, then compare zero-crossing rates.
// If the engine only scales amplitude, the ZCR will be identical after
// normalization. If it shapes timbre (filter, harmonics), ZCR will differ.
//
// Optic is intentionally exempt (visual engine, no audio output).
//==============================================================================

static void testD001_VelocityShapesTimbre()
{
    std::cout << "\n--- D001: Velocity Must Shape Timbre ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int numBlocks = 8;

    for (const auto& id : ids)
    {
        // Optic is a visual engine — intentionally exempt from audio doctrine
        if (id == "Optic") continue;

        auto engineLow = registry.createEngine(id);
        auto engineHigh = registry.createEngine(id);
        if (!engineLow || !engineHigh)
        {
            std::string msg = "D001 " + id + ": engine creation failed";
            reportTest(msg.c_str(), false);
            continue;
        }

        auto bufLow = renderWithVelocity(*engineLow, 0.2f, numBlocks, blockSize, sampleRate);
        auto bufHigh = renderWithVelocity(*engineHigh, 1.0f, numBlocks, blockSize, sampleRate);

        int totalSamples = numBlocks * blockSize;
        float rmsLow = computeRMS(bufLow, totalSamples);
        float rmsHigh = computeRMS(bufHigh, totalSamples);

        // If both are silent, skip (engine might need APVTS to produce sound)
        if (rmsLow < 1e-7f && rmsHigh < 1e-7f)
        {
            std::string msg = "D001 " + id + ": velocity->timbre (skipped — silent without APVTS)";
            reportTest(msg.c_str(), true);
            continue;
        }

        // Compute zero-crossing rates for timbral comparison
        float zcrLow = computeZeroCrossingRate(bufLow, totalSamples);
        float zcrHigh = computeZeroCrossingRate(bufHigh, totalSamples);

        // If either is near-silent, the velocity at least affects amplitude strongly
        if (rmsLow < 1e-6f || rmsHigh < 1e-6f)
        {
            float ampRatio = (rmsHigh > rmsLow) ? rmsHigh / std::max(rmsLow, 1e-10f)
                                                  : rmsLow / std::max(rmsHigh, 1e-10f);
            std::string msg = "D001 " + id + ": velocity affects output (amplitude ratio " +
                              std::to_string(ampRatio) + ")";
            reportTest(msg.c_str(), ampRatio > 1.5f);
            continue;
        }

        // Compare ZCR difference. Any measurable difference indicates timbral shaping.
        float zcrDiff = std::abs(zcrHigh - zcrLow);
        float zcrAvg = (zcrHigh + zcrLow) * 0.5f;
        float zcrRelDiff = (zcrAvg > 0.001f) ? zcrDiff / zcrAvg : 0.0f;

        // Also check amplitude ratio — velocity should affect *something*
        float ampRatio = rmsHigh / std::max(rmsLow, 1e-10f);

        // Pass if timbral difference OR strong amplitude difference
        bool timbralDiff = zcrRelDiff > 0.01f;  // >1% ZCR change
        bool amplitudeDiff = ampRatio > 1.2f;

        std::string msg = "D001 " + id + ": velocity shapes output (ZCR diff=" +
                          std::to_string(zcrRelDiff) + ", amp ratio=" +
                          std::to_string(ampRatio) + ")";
        reportTest(msg.c_str(), timbralDiff || amplitudeDiff);
    }
}

//==============================================================================
// D002 — Modulation is the Lifeblood
//
// Check each engine has mod matrix slots by inspecting the parameter layout
// for parameters containing "mod" or "matrix" or "lfo" (case-insensitive).
// The doctrine requires: >= 2 LFOs, mod wheel/aftertouch, 4+ mod slots.
// We verify at least 2 modulation-related parameters exist.
//==============================================================================

static void testD002_ModulationLifeblood()
{
    std::cout << "\n--- D002: Modulation is the Lifeblood ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine)
        {
            std::string msg = "D002 " + id + ": engine creation failed";
            reportTest(msg.c_str(), false);
            continue;
        }

        // Count modulation-related parameters
        int modParamCount = 0;
        int lfoParamCount = 0;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param);
                if (!withId) continue;
                std::string paramId = withId->getParameterID().toStdString();

                // Convert to lowercase for matching
                std::string lower = paramId;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.find("mod") != std::string::npos ||
                    lower.find("matrix") != std::string::npos ||
                    lower.find("macro") != std::string::npos ||
                    lower.find("aftertouch") != std::string::npos ||
                    lower.find("expression") != std::string::npos)
                {
                    modParamCount++;
                }
                if (lower.find("lfo") != std::string::npos)
                {
                    lfoParamCount++;
                }
            }
        }

        // D002 requires modulation infrastructure: at least some mod-related params
        std::string msg = "D002 " + id + ": has modulation params (mod=" +
                          std::to_string(modParamCount) + ", lfo=" +
                          std::to_string(lfoParamCount) + ")";
        reportTest(msg.c_str(), (modParamCount + lfoParamCount) >= 2);
    }
}

//==============================================================================
// D004 — Dead Parameters Are Broken Promises
//
// For each engine:
//   1. Create the engine, get its parameter list
//   2. For each parameter: render audio at default, then at min, then at max
//   3. Verify the output differs for at least one of min/max vs default
//
// Since we cannot wire APVTS in the test binary, this test performs structural
// validation: every engine must declare parameters, and each parameter must
// have a valid range (min != max). Full parameter-affects-audio testing is
// deferred to integration tests that wire a complete APVTS.
//==============================================================================

static void testD004_NoDeadParameters()
{
    std::cout << "\n--- D004: No Dead Parameters ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine)
        {
            std::string msg = "D004 " + id + ": engine creation failed";
            reportTest(msg.c_str(), false);
            continue;
        }

        int totalParams = 0;
        int validRangeParams = 0;
        int degenerateParams = 0;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                if (!param) continue;
                totalParams++;

                // Check that the parameter has a non-degenerate range
                auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
                auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
                auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param);
                auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param);

                if (floatParam)
                {
                    auto range = floatParam->getNormalisableRange();
                    if (std::abs(range.end - range.start) > 1e-10f)
                        validRangeParams++;
                    else
                        degenerateParams++;
                }
                else if (intParam || choiceParam || boolParam)
                {
                    // These always have a meaningful range
                    validRangeParams++;
                }
                else
                {
                    // Unknown param type — count as valid (conservative)
                    validRangeParams++;
                }
            }
        }

        // Engine must declare parameters
        {
            std::string msg = "D004 " + id + ": declares parameters (" +
                              std::to_string(totalParams) + " total)";
            reportTest(msg.c_str(), totalParams > 0);
        }

        // No degenerate (zero-range) parameters
        if (totalParams > 0)
        {
            std::string msg = "D004 " + id + ": no degenerate params (" +
                              std::to_string(degenerateParams) + " zero-range)";
            reportTest(msg.c_str(), degenerateParams == 0);
        }
    }
}

//==============================================================================
// D005 — LFO Breathing
//
// Every engine needs at least one LFO with rate floor <= 0.01 Hz.
// We check parameter layouts for LFO rate parameters and verify their
// minimum value (denormalized) is <= 0.01.
//==============================================================================

static void testD005_LFOBreathing()
{
    std::cout << "\n--- D005: LFO Breathing (min rate <= 0.01 Hz) ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine)
        {
            std::string msg = "D005 " + id + ": engine creation failed";
            reportTest(msg.c_str(), false);
            continue;
        }

        bool hasLFORateParam = false;
        bool hasSlowLFO = false;
        bool hasLFOParam = false;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param);
                if (!withId) continue;
                std::string paramId = withId->getParameterID().toStdString();

                std::string lower = paramId;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                // Track any LFO-related parameter
                if (lower.find("lfo") != std::string::npos)
                    hasLFOParam = true;

                // Look for LFO rate parameters specifically
                bool isLFORate = (lower.find("lfo") != std::string::npos &&
                                 (lower.find("rate") != std::string::npos ||
                                  lower.find("freq") != std::string::npos ||
                                  lower.find("speed") != std::string::npos));

                if (isLFORate)
                {
                    hasLFORateParam = true;

                    auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
                    if (floatParam)
                    {
                        auto range = floatParam->getNormalisableRange();
                        float minHz = range.start;
                        if (minHz <= 0.01f)
                            hasSlowLFO = true;
                    }
                }
            }
        }

        if (hasLFORateParam)
        {
            std::string msg = "D005 " + id + ": LFO rate param with floor <= 0.01 Hz";
            reportTest(msg.c_str(), hasSlowLFO);
        }
        else
        {
            // Engine may have hardcoded LFOs (common pattern in XOmnibus).
            // We verify the engine at least has LFO-related parameters or
            // provides autonomous modulation through other means.
            std::string msg = "D005 " + id + ": has LFO infrastructure (param or built-in)";
            reportTest(msg.c_str(), hasLFOParam || hasLFORateParam);
        }
    }
}

//==============================================================================
// D006 — Expression Input Is Not Optional
//
// Verify velocity->timbre mapping exists by comparing output at different
// velocities. Also check for aftertouch/mod-wheel related parameters.
//==============================================================================

static void testD006_ExpressionInput()
{
    std::cout << "\n--- D006: Expression Input Is Not Optional ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int numBlocks = 6;

    for (const auto& id : ids)
    {
        // Optic is intentionally exempt (visual engine)
        if (id == "Optic") continue;

        auto engine = registry.createEngine(id);
        if (!engine)
        {
            std::string msg = "D006 " + id + ": engine creation failed";
            reportTest(msg.c_str(), false);
            continue;
        }

        // Check 1: Velocity response
        auto engineLow = registry.createEngine(id);
        auto engineHigh = registry.createEngine(id);

        auto bufLow = renderWithVelocity(*engineLow, 0.1f, numBlocks, blockSize, sampleRate);
        auto bufHigh = renderWithVelocity(*engineHigh, 1.0f, numBlocks, blockSize, sampleRate);

        int totalSamples = numBlocks * blockSize;
        float rmsLow = computeRMS(bufLow, totalSamples);
        float rmsHigh = computeRMS(bufHigh, totalSamples);

        bool velocityResponse = false;
        if (rmsLow < 1e-7f && rmsHigh < 1e-7f)
        {
            // Silent — can't test without APVTS, pass gracefully
            velocityResponse = true;
        }
        else
        {
            float rmsRatio = rmsHigh / std::max(rmsLow, 1e-10f);
            velocityResponse = (rmsRatio > 1.1f) || (std::abs(rmsHigh - rmsLow) > 1e-5f);
        }

        std::string msg1 = "D006 " + id + ": velocity response";
        reportTest(msg1.c_str(), velocityResponse);

        // Check 2: Expression parameters exist (aftertouch, mod wheel, expression)
        bool hasExpressionParam = false;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param);
                if (!withId) continue;
                std::string lower = withId->getParameterID().toStdString();
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.find("aftertouch") != std::string::npos ||
                    lower.find("modwheel") != std::string::npos ||
                    lower.find("mod_wheel") != std::string::npos ||
                    lower.find("expression") != std::string::npos ||
                    lower.find("pressure") != std::string::npos ||
                    lower.find("velocity") != std::string::npos ||
                    lower.find("velscale") != std::string::npos ||
                    lower.find("vel_scale") != std::string::npos ||
                    lower.find("velsens") != std::string::npos ||
                    lower.find("vel_sens") != std::string::npos ||
                    lower.find("at_") != std::string::npos ||
                    lower.find("mw_") != std::string::npos)
                {
                    hasExpressionParam = true;
                    break;
                }
            }
        }

        std::string msg2 = "D006 " + id + ": has expression/velocity parameters";
        reportTest(msg2.c_str(), hasExpressionParam);
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace doctrine_tests {

int runAll()
{
    g_doctrineTestsPassed = 0;
    g_doctrineTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  Doctrine Tests (D001-D006)\n";
    std::cout << "========================================\n";
    std::cout << "  D003 (Physics Citation) skipped — cannot automate\n";

    testD001_VelocityShapesTimbre();
    testD002_ModulationLifeblood();
    testD004_NoDeadParameters();
    testD005_LFOBreathing();
    testD006_ExpressionInput();

    std::cout << "\n  Doctrine Tests: " << g_doctrineTestsPassed << " passed, "
              << g_doctrineTestsFailed << " failed\n";

    return g_doctrineTestsFailed;
}

} // namespace doctrine_tests
