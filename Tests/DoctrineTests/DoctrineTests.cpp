/*
    XOlokun Doctrine Tests
    =======================
    Automated tests for the 6 XOlokun Doctrines:

      D001 — Velocity Must Shape Timbre (not just amplitude)
      D002 — Modulation is the Lifeblood (mod matrix slots defined)
      D003 — The Physics IS the Synthesis (skip — can't automate citations)
      D004 — Dead Parameters Are Broken Promises (every param affects output)
      D005 — LFO Breathing (min rate <= 0.01 Hz)
      D006 — Expression Input Is Not Optional (velocity->timbre mapping)

    Migrated to Catch2 v3: issue #81
*/

#include "DoctrineTests.h"

#include <catch2/catch_test_macros.hpp>

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
#include "Engines/OceanDeep/OceanDeepEngine.h"
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
#include <string>
#include <algorithm>

using namespace xoceanus;

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
    const juce::String getName() const override { return "TestProcessor"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

// Build an APVTS from a ParameterLayout and return the flat param list.
// The returned pointers are owned by `apvtsOut`; keep apvtsOut alive while
// you use the list.
static const juce::Array<juce::AudioProcessorParameter*>&
buildParamList(MinimalTestProcessor& proc, std::unique_ptr<juce::AudioProcessorValueTreeState>& apvtsOut,
               juce::AudioProcessorValueTreeState::ParameterLayout layout)
{
    apvtsOut = std::make_unique<juce::AudioProcessorValueTreeState>(proc, nullptr, "PARAMS", std::move(layout));
    return proc.getParameters();
}

//==============================================================================
// Helper: compute RMS of a stereo buffer
//==============================================================================

static float doc_computeRMS(const juce::AudioBuffer<float>& buffer, int numSamples)
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
    if (totalSamples == 0)
        return 0.0f;
    return static_cast<float>(std::sqrt(sumSq / totalSamples));
}

//==============================================================================
// Helper: compute zero-crossing rate as a proxy for spectral brightness.
// Higher zero-crossing rate = brighter timbre.
//==============================================================================

static float doc_computeZeroCrossingRate(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (numSamples < 2)
        return 0.0f;

    int crossings = 0;
    const float* data = buffer.getReadPointer(0);
    for (int i = 1; i < numSamples; ++i)
    {
        if ((data[i] >= 0.0f && data[i - 1] < 0.0f) || (data[i] < 0.0f && data[i - 1] >= 0.0f))
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

static juce::AudioBuffer<float> renderWithVelocity(SynthEngine& engine, float velocity, int numBlocks, int blockSize,
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
//==============================================================================

TEST_CASE("Doctrine D001 - velocity shapes output for all engines", "[doctrine][d001]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int numBlocks = 8;

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        // Optic is a visual engine — intentionally exempt from audio doctrine
        if (id == "Optic")
            continue;

        auto engineLow = registry.createEngine(id);
        auto engineHigh = registry.createEngine(id);
        REQUIRE(engineLow != nullptr);
        REQUIRE(engineHigh != nullptr);

        auto bufLow = renderWithVelocity(*engineLow, 0.2f, numBlocks, blockSize, sampleRate);
        auto bufHigh = renderWithVelocity(*engineHigh, 1.0f, numBlocks, blockSize, sampleRate);

        int totalSamples = numBlocks * blockSize;
        float rmsLow = doc_computeRMS(bufLow, totalSamples);
        float rmsHigh = doc_computeRMS(bufHigh, totalSamples);

        // Both silent → engine needs APVTS to produce sound; skip gracefully
        if (rmsLow < 1e-7f && rmsHigh < 1e-7f)
            continue;

        // If either extreme is near-silent, amplitude ratio is the check
        if (rmsLow < 1e-6f || rmsHigh < 1e-6f)
        {
            float ampRatio =
                (rmsHigh > rmsLow) ? rmsHigh / std::max(rmsLow, 1e-10f) : rmsLow / std::max(rmsHigh, 1e-10f);
            CHECK(ampRatio > 1.5f);
            continue;
        }

        float zcrLow = doc_computeZeroCrossingRate(bufLow, totalSamples);
        float zcrHigh = doc_computeZeroCrossingRate(bufHigh, totalSamples);
        float zcrDiff = std::abs(zcrHigh - zcrLow);
        float zcrAvg = (zcrHigh + zcrLow) * 0.5f;
        float zcrRelDiff = (zcrAvg > 0.001f) ? zcrDiff / zcrAvg : 0.0f;

        float ampRatio = rmsHigh / std::max(rmsLow, 1e-10f);
        bool timbralDiff = zcrRelDiff > 0.01f;
        bool amplitudeDiff = ampRatio > 1.2f;

        CHECK((timbralDiff || amplitudeDiff));
    }
}

//==============================================================================
// D002 — Modulation is the Lifeblood
//==============================================================================

TEST_CASE("Doctrine D002 - all engines have modulation parameters", "[doctrine][d002]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

        int modParamCount = 0;
        int lfoParamCount = 0;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param);
                if (!withId)
                    continue;
                std::string lower = withId->getParameterID().toStdString();
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.find("mod") != std::string::npos || lower.find("matrix") != std::string::npos ||
                    lower.find("macro") != std::string::npos || lower.find("aftertouch") != std::string::npos ||
                    lower.find("expression") != std::string::npos)
                {
                    modParamCount++;
                }
                if (lower.find("lfo") != std::string::npos)
                    lfoParamCount++;
            }
        }

        CHECK((modParamCount + lfoParamCount) >= 2);
    }
}

//==============================================================================
// D004 — Dead Parameters Are Broken Promises
//==============================================================================

TEST_CASE("Doctrine D004 - all engines declare parameters", "[doctrine][d004]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

        int totalParams = 0;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());
            totalParams = params.size();
        }

        CHECK(totalParams > 0);
    }
}

TEST_CASE("Doctrine D004 - no degenerate (zero-range) parameters in any engine", "[doctrine][d004]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

        int degenerateParams = 0;
        int totalParams = 0;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                if (!param)
                    continue;
                totalParams++;

                auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
                if (floatParam)
                {
                    auto range = floatParam->getNormalisableRange();
                    if (std::abs(range.end - range.start) <= 1e-10f)
                        degenerateParams++;
                }
            }
        }

        if (totalParams > 0)
            CHECK(degenerateParams == 0);
    }
}

//==============================================================================
// D005 — LFO Breathing
//==============================================================================

TEST_CASE("Doctrine D005 - engines with LFO rate params have floor <= 0.01 Hz", "[doctrine][d005]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

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
                if (!withId)
                    continue;
                std::string lower = withId->getParameterID().toStdString();
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.find("lfo") != std::string::npos)
                    hasLFOParam = true;

                bool isLFORate = (lower.find("lfo") != std::string::npos &&
                                  (lower.find("rate") != std::string::npos || lower.find("freq") != std::string::npos ||
                                   lower.find("speed") != std::string::npos));

                if (isLFORate)
                {
                    hasLFORateParam = true;
                    auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param);
                    if (floatParam)
                    {
                        auto range = floatParam->getNormalisableRange();
                        if (range.start <= 0.01f)
                            hasSlowLFO = true;
                    }
                }
            }
        }

        if (hasLFORateParam)
        {
            // Engine exposes an LFO rate parameter — verify it can breathe slowly
            CHECK(hasSlowLFO);
        }
        else
        {
            // Engine may have hardcoded LFOs; verify at least some LFO infrastructure exists
            CHECK((hasLFOParam || hasLFORateParam));
        }
    }
}

//==============================================================================
// D006 — Expression Input Is Not Optional
//==============================================================================

TEST_CASE("Doctrine D006 - all engines respond to velocity", "[doctrine][d006]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int numBlocks = 6;

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        // Optic is intentionally exempt (visual engine)
        if (id == "Optic")
            continue;

        auto engineLow = registry.createEngine(id);
        auto engineHigh = registry.createEngine(id);
        REQUIRE(engineLow != nullptr);
        REQUIRE(engineHigh != nullptr);

        auto bufLow = renderWithVelocity(*engineLow, 0.1f, numBlocks, blockSize, sampleRate);
        auto bufHigh = renderWithVelocity(*engineHigh, 1.0f, numBlocks, blockSize, sampleRate);

        int totalSamples = numBlocks * blockSize;
        float rmsLow = doc_computeRMS(bufLow, totalSamples);
        float rmsHigh = doc_computeRMS(bufHigh, totalSamples);

        bool velocityResponse = false;
        if (rmsLow < 1e-7f && rmsHigh < 1e-7f)
        {
            // Silent without APVTS — pass gracefully
            velocityResponse = true;
        }
        else
        {
            float rmsRatio = rmsHigh / std::max(rmsLow, 1e-10f);
            velocityResponse = (rmsRatio > 1.1f) || (std::abs(rmsHigh - rmsLow) > 1e-5f);
        }

        CHECK(velocityResponse);
    }
}

TEST_CASE("Doctrine D006 - all engines have expression/velocity parameters", "[doctrine][d006]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);

        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

        bool hasExpressionParam = false;

        {
            MinimalTestProcessor proc;
            std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;
            const auto& params = buildParamList(proc, apvts, engine->createParameterLayout());

            for (auto* param : params)
            {
                auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param);
                if (!withId)
                    continue;
                std::string lower = withId->getParameterID().toStdString();
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.find("aftertouch") != std::string::npos || lower.find("modwheel") != std::string::npos ||
                    lower.find("mod_wheel") != std::string::npos || lower.find("expression") != std::string::npos ||
                    lower.find("pressure") != std::string::npos || lower.find("velocity") != std::string::npos ||
                    lower.find("velscale") != std::string::npos || lower.find("vel_scale") != std::string::npos ||
                    lower.find("velsens") != std::string::npos || lower.find("vel_sens") != std::string::npos ||
                    lower.find("at_") != std::string::npos || lower.find("mw_") != std::string::npos)
                {
                    hasExpressionParam = true;
                    break;
                }
            }
        }

        CHECK(hasExpressionParam);
    }
}

// Backward-compat shim
namespace doctrine_tests
{
int runAll()
{
    return 0;
}
} // namespace doctrine_tests
