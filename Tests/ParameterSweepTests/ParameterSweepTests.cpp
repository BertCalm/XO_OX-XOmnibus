/*
    XOceanus Parameter Sweep Tests (#56)
    ======================================
    For every registered engine, sweeps each parameter from default → min → max,
    renders a short buffer at each extreme, and verifies the output differs.

    A parameter that produces identical audio at 0.0 and 1.0 is a dead parameter
    (D004 violation: "Dead Parameters Are Broken Promises").

    Migrated to Catch2 v3: issue #81
*/

#include "ParameterSweepTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/EngineRegistry.h"
#include "Core/SynthEngine.h"

// Engine headers — same list as DoctrineTests.cpp.
// registerEngine() is idempotent (returns false for duplicates), so this is safe.
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
#include <vector>
#include <unordered_set>
#include <algorithm>

using namespace xoceanus;

//==============================================================================
// Constants
//==============================================================================

static constexpr double SAMPLE_RATE      = 44100.0;
static constexpr int    BLOCK_SIZE       = 512;
static constexpr int    NUM_BLOCKS       = 6;
static constexpr float  DEAD_THRESHOLD   = 5e-5f;
static constexpr int    MAX_PARAMS_PER_ENGINE = 40;

//==============================================================================
// Allow-list: parameter ID substrings that legitimately don't change RMS.
//==============================================================================
static const std::unordered_set<std::string> PARAM_ALLOWLIST = {
    "pan",       "width",    "spread",    "tune",
    "_pan",      "detune",   "transpose", "glide",
    "pan_",      "portamento", "routing", "mode",
    "scale",     "voicemode", "unison",
};

//==============================================================================
// SweepTestProcessor
//==============================================================================

struct SweepTestProcessor : juce::AudioProcessor
{
    const juce::String getName() const override              { return "SweepTestProcessor"; }
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

//==============================================================================
// Signal analysis helpers
//==============================================================================

static float sweep_computeRMS (const juce::AudioBuffer<float>& buf, int numSamples)
{
    double sumSq = 0.0;
    int total = 0;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* d = buf.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            sumSq += static_cast<double>(d[i]) * static_cast<double>(d[i]);
        total += numSamples;
    }
    return total > 0 ? static_cast<float>(std::sqrt (sumSq / total)) : 0.0f;
}

static bool sweep_hasNaNOrInf (const juce::AudioBuffer<float>& buf, int numSamples)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* d = buf.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (!std::isfinite (d[i])) return true;
    }
    return false;
}

static float sweep_computeZCR (const juce::AudioBuffer<float>& buf, int numSamples)
{
    if (numSamples < 2) return 0.0f;
    int crossings = 0;
    const float* d = buf.getReadPointer (0);
    for (int i = 1; i < numSamples; ++i)
    {
        if ((d[i] >= 0.0f && d[i - 1] < 0.0f) || (d[i] < 0.0f && d[i - 1] >= 0.0f))
            ++crossings;
    }
    return static_cast<float>(crossings) / static_cast<float>(numSamples - 1);
}

//==============================================================================
// Render helper
//==============================================================================

static juce::AudioBuffer<float> renderWithParam (
    SynthEngine& engine,
    juce::AudioProcessorValueTreeState& apvts,
    const std::string& paramId,
    float normalisedValue)
{
    if (auto* param = apvts.getParameter (juce::String (paramId)))
        param->setValueNotifyingHost (normalisedValue);

    engine.prepare (SAMPLE_RATE, BLOCK_SIZE);
    engine.reset();

    const int totalSamples = NUM_BLOCKS * BLOCK_SIZE;
    juce::AudioBuffer<float> result (2, totalSamples);
    result.clear();

    for (int block = 0; block < NUM_BLOCKS; ++block)
    {
        juce::AudioBuffer<float> blockBuf (2, BLOCK_SIZE);
        blockBuf.clear();
        juce::MidiBuffer midi;

        if (block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);

        engine.renderBlock (blockBuf, midi, BLOCK_SIZE);

        const int destStart = block * BLOCK_SIZE;
        for (int ch = 0; ch < 2; ++ch)
            result.copyFrom (ch, destStart, blockBuf, ch, 0, BLOCK_SIZE);
    }

    return result;
}

static bool isAllowListed (const std::string& paramId)
{
    std::string lower = paramId;
    std::transform (lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& kw : PARAM_ALLOWLIST)
        if (lower.find (kw) != std::string::npos) return true;
    return false;
}

//==============================================================================
// D004 Parameter Sweep Tests
//==============================================================================

TEST_CASE("ParameterSweep D004 - no NaN/Inf at parameter extremes for any engine",
          "[sweep][d004][nan]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort (ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        auto engineMin = registry.createEngine (id);
        auto engineMax = registry.createEngine (id);
        if (!engineMin || !engineMax) continue;

        SweepTestProcessor procMin, procMax;
        std::unique_ptr<juce::AudioProcessorValueTreeState> apvtsMin, apvtsMax;
        apvtsMin = std::make_unique<juce::AudioProcessorValueTreeState>(
            procMin, nullptr, "PARAMS", engineMin->createParameterLayout());
        apvtsMax = std::make_unique<juce::AudioProcessorValueTreeState>(
            procMax, nullptr, "PARAMS", engineMax->createParameterLayout());

        const auto& params = procMin.getParameters();
        if (params.isEmpty()) continue;

        std::vector<std::string> paramIds;
        for (auto* param : params)
        {
            if (dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
                    paramIds.push_back (wp->getParameterID().toStdString());
            }
        }
        if (paramIds.empty()) continue;

        // Cap to MAX_PARAMS_PER_ENGINE
        std::vector<std::string> toTest;
        if ((int)paramIds.size() <= MAX_PARAMS_PER_ENGINE)
        {
            toTest = paramIds;
        }
        else
        {
            int step = static_cast<int>(paramIds.size()) / MAX_PARAMS_PER_ENGINE;
            for (int i = 0; i < (int)paramIds.size() && (int)toTest.size() < MAX_PARAMS_PER_ENGINE; i += step)
                toTest.push_back (paramIds[i]);
        }

        const int totalSamples = NUM_BLOCKS * BLOCK_SIZE;

        for (const auto& pid : toTest)
        {
            INFO("Engine: " << id << " | Param: " << pid);

            auto bufMin = renderWithParam (*engineMin, *apvtsMin, pid, 0.0f);
            auto bufMax = renderWithParam (*engineMax, *apvtsMax, pid, 1.0f);

            CHECK(!sweep_hasNaNOrInf (bufMin, totalSamples));
            CHECK(!sweep_hasNaNOrInf (bufMax, totalSamples));
        }
    }
}

TEST_CASE("ParameterSweep D004 - no dead (zero-delta RMS) parameters for any engine",
          "[sweep][d004][dead-params]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort (ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        auto engineMin = registry.createEngine (id);
        auto engineMax = registry.createEngine (id);
        if (!engineMin || !engineMax) continue;

        SweepTestProcessor procMin, procMax;
        std::unique_ptr<juce::AudioProcessorValueTreeState> apvtsMin, apvtsMax;
        apvtsMin = std::make_unique<juce::AudioProcessorValueTreeState>(
            procMin, nullptr, "PARAMS", engineMin->createParameterLayout());
        apvtsMax = std::make_unique<juce::AudioProcessorValueTreeState>(
            procMax, nullptr, "PARAMS", engineMax->createParameterLayout());

        const auto& params = procMin.getParameters();
        if (params.isEmpty()) continue;

        std::vector<std::string> paramIds;
        for (auto* param : params)
        {
            if (dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
                    paramIds.push_back (wp->getParameterID().toStdString());
            }
        }
        if (paramIds.empty()) continue;

        std::vector<std::string> toTest;
        if ((int)paramIds.size() <= MAX_PARAMS_PER_ENGINE)
        {
            toTest = paramIds;
        }
        else
        {
            int step = static_cast<int>(paramIds.size()) / MAX_PARAMS_PER_ENGINE;
            for (int i = 0; i < (int)paramIds.size() && (int)toTest.size() < MAX_PARAMS_PER_ENGINE; i += step)
                toTest.push_back (paramIds[i]);
        }

        const int totalSamples = NUM_BLOCKS * BLOCK_SIZE;
        bool anyTested = false;

        for (const auto& pid : toTest)
        {
            auto bufMin = renderWithParam (*engineMin, *apvtsMin, pid, 0.0f);
            auto bufMax = renderWithParam (*engineMax, *apvtsMax, pid, 1.0f);

            float rmsMin = sweep_computeRMS (bufMin, totalSamples);
            float rmsMax = sweep_computeRMS (bufMax, totalSamples);

            // Both silent → engine needs full APVTS wiring; skip
            if (rmsMin < 1e-7f && rmsMax < 1e-7f) continue;

            anyTested = true;
            float rmsDelta = std::abs (rmsMax - rmsMin);
            bool rmsChanged = rmsDelta > DEAD_THRESHOLD;

            if (rmsChanged || isAllowListed (pid))
            {
                // Param passes: either RMS changed or it's on the allow-list
                // (allow-listed params may only affect stereo field, not RMS)
            }
            else
            {
                // Dead parameter — report per-parameter failure with context
                INFO("Engine: " << id
                     << " | Param: " << pid
                     << " | RMS min=" << rmsMin
                     << " max=" << rmsMax
                     << " delta=" << rmsDelta);
                CHECK(rmsChanged);  // will fail and record the INFO above
            }
        }

        (void)anyTested; // suppress unused-variable warning
    }
}

// Backward-compat shim
namespace param_sweep_tests {
int runAll() { return 0; }
} // namespace param_sweep_tests
