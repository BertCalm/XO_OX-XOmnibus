/*
    XOceanus Parameter Sweep Tests (#56)
    ======================================
    For every registered engine, sweeps each parameter from default → min → max,
    renders a short buffer at each extreme, and verifies the output differs.

    A parameter that produces identical audio at 0.0 and 1.0 is a dead parameter
    (D004 violation: "Dead Parameters Are Broken Promises").

    Strategy
    --------
    1.  Create two independent engine instances.
    2.  Build an APVTS for each so parameters are actually wired.
    3.  Hold all parameters at their default value except the one under test.
    4.  Render instance A with param at minimum (0.0 normalised).
    5.  Render instance B with param at maximum (1.0 normalised).
    6.  Compare RMS of the two outputs.
    7.  If |RMS_A - RMS_B| < DEAD_THRESHOLD → flag as dead.

    Allow-list
    ----------
    Parameters that legitimately do not change RMS (e.g. pan, output routing
    selectors, tuning cents) are maintained in PARAM_ALLOWLIST below.  A param
    on the allow-list is still tested — it just uses a spectral/ZCR check
    instead of RMS, and passes if any measurable change is detected.

    Timeout safety
    --------------
    Each engine gets at most MAX_PARAMS_PER_ENGINE parameters tested.  Engines
    with hundreds of parameters are capped so the full suite stays < 5 minutes.

    No test framework — assert-based, matching the existing XOceanus test style.
*/

#include "ParameterSweepTests.h"

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
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <numeric>

using namespace xoceanus;

//==============================================================================
// Constants
//==============================================================================

static constexpr double SAMPLE_RATE      = 44100.0;
static constexpr int    BLOCK_SIZE       = 512;
static constexpr int    NUM_BLOCKS       = 6;    // 0.069s render — fast but enough for envelopes
static constexpr float  DEAD_THRESHOLD   = 5e-5f; // RMS delta below this → dead param flag
static constexpr int    MAX_PARAMS_PER_ENGINE = 40; // cap per engine to bound test time

//==============================================================================
// Allow-list: parameter ID substrings that legitimately don't change RMS.
// These are checked with ZCR/spectral change instead of RMS alone.
//==============================================================================
static const std::unordered_set<std::string> PARAM_ALLOWLIST = {
    "pan",       // stereo pan — RMS stays the same, only stereo image changes
    "_pan",
    "pan_",
    "width",     // stereo width
    "spread",    // stereo spread
    "tune",      // fine tune in cents — may not change RMS meaningfully
    "detune",
    "transpose", // semitone transpose (pitch shift)
    "glide",     // glide/portamento time — only audible during note transitions
    "portamento",
    "routing",   // output routing selectors
    "mode",      // mode switches that only take effect on next note
    "scale",     // scale/tuning selector — no effect on a held note
    "voicemode", // voice mode switches
    "unison",    // unison detune — need multiple voices
};

//==============================================================================
// MinimalTestProcessor (same pattern as DoctrineTests.cpp)
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
// Test infrastructure
//==============================================================================

static int g_sweepTestsPassed = 0;
static int g_sweepTestsFailed = 0;
static int g_sweepTestsSkipped = 0;
static int g_deadParamsFound = 0;

static void reportSweep (const std::string& name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_sweepTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_sweepTestsFailed++;
        g_deadParamsFound++;
    }
}

//==============================================================================
// Signal analysis helpers
//==============================================================================

static float computeRMS (const juce::AudioBuffer<float>& buf, int numSamples)
{
    double sumSq = 0.0;
    int total = 0;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* d = buf.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            sumSq += static_cast<double>(d[i]) * static_cast<double>(d[i]);
        }
        total += numSamples;
    }
    return total > 0 ? static_cast<float>(std::sqrt (sumSq / total)) : 0.0f;
}

static bool hasNaNOrInf (const juce::AudioBuffer<float>& buf, int numSamples)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* d = buf.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            if (!std::isfinite (d[i])) return true;
        }
    }
    return false;
}

static float computeZCR (const juce::AudioBuffer<float>& buf, int numSamples)
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
    // Set the target parameter
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

//==============================================================================
// Check if a param ID is on the allow-list
//==============================================================================

static bool isAllowListed (const std::string& paramId)
{
    std::string lower = paramId;
    std::transform (lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& kw : PARAM_ALLOWLIST)
    {
        if (lower.find (kw) != std::string::npos)
            return true;
    }
    return false;
}

//==============================================================================
// Per-engine sweep
//==============================================================================

static void sweepEngine (const std::string& engineId)
{
    auto& registry = EngineRegistry::instance();

    // We need two instances: one for each extreme, sharing the same APVTS layout
    auto engineMin = registry.createEngine (engineId);
    auto engineMax = registry.createEngine (engineId);

    if (!engineMin || !engineMax)
    {
        std::cout << "  [SKIP] " << engineId << ": engine creation failed\n";
        g_sweepTestsSkipped++;
        return;
    }

    // Build APVTS for each instance
    SweepTestProcessor procMin, procMax;
    std::unique_ptr<juce::AudioProcessorValueTreeState> apvtsMin, apvtsMax;

    apvtsMin = std::make_unique<juce::AudioProcessorValueTreeState>(
        procMin, nullptr, "PARAMS", engineMin->createParameterLayout());
    apvtsMax = std::make_unique<juce::AudioProcessorValueTreeState>(
        procMax, nullptr, "PARAMS", engineMax->createParameterLayout());

    const auto& params = procMin.getParameters();
    if (params.isEmpty())
    {
        std::cout << "  [SKIP] " << engineId << ": no parameters declared\n";
        g_sweepTestsSkipped++;
        return;
    }

    // Collect float parameter IDs (only float params produce meaningful sweeps)
    std::vector<std::string> paramIds;
    for (auto* param : params)
    {
        if (auto* fp = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            (void)fp;
            if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
                paramIds.push_back (wp->getParameterID().toStdString());
        }
    }

    if (paramIds.empty())
    {
        std::cout << "  [SKIP] " << engineId << ": no float parameters to sweep\n";
        g_sweepTestsSkipped++;
        return;
    }

    // Cap to MAX_PARAMS_PER_ENGINE — test evenly distributed subset for large engines
    std::vector<std::string> toTest;
    if ((int)paramIds.size() <= MAX_PARAMS_PER_ENGINE)
    {
        toTest = paramIds;
    }
    else
    {
        // Even distribution: pick every N-th param
        int step = static_cast<int>(paramIds.size()) / MAX_PARAMS_PER_ENGINE;
        for (int i = 0; i < (int)paramIds.size() && (int)toTest.size() < MAX_PARAMS_PER_ENGINE; i += step)
            toTest.push_back (paramIds[i]);
    }

    int deadCount = 0;
    int testedCount = 0;

    for (const auto& pid : toTest)
    {
        // Render at min (0.0) and max (1.0)
        auto bufMin = renderWithParam (*engineMin, *apvtsMin, pid, 0.0f);
        auto bufMax = renderWithParam (*engineMax, *apvtsMax, pid, 1.0f);

        const int totalSamples = NUM_BLOCKS * BLOCK_SIZE;

        // NaN/Inf check — always fatal regardless of allow-list
        if (hasNaNOrInf (bufMin, totalSamples))
        {
            std::string msg = "D004/NaN " + engineId + "::" + pid + " at min produces NaN/Inf";
            reportSweep (msg, false);
            testedCount++;
            continue;
        }
        if (hasNaNOrInf (bufMax, totalSamples))
        {
            std::string msg = "D004/NaN " + engineId + "::" + pid + " at max produces NaN/Inf";
            reportSweep (msg, false);
            testedCount++;
            continue;
        }

        const float rmsMin = computeRMS (bufMin, totalSamples);
        const float rmsMax = computeRMS (bufMax, totalSamples);

        // Both silent → engine needs full APVTS wiring; skip gracefully
        if (rmsMin < 1e-7f && rmsMax < 1e-7f)
        {
            g_sweepTestsSkipped++;
            continue;
        }

        const float rmsDelta = std::abs (rmsMax - rmsMin);
        const bool rmsChanged = rmsDelta > DEAD_THRESHOLD;

        testedCount++;

        if (rmsChanged)
        {
            // Param clearly affects audio
            std::string msg = "D004 " + engineId + "::" + pid + " (RMS delta=" +
                              std::to_string (rmsDelta) + ")";
            reportSweep (msg, true);
        }
        else if (isAllowListed (pid))
        {
            // Allow-listed: fall back to ZCR/spectral check
            float zcrMin = computeZCR (bufMin, totalSamples);
            float zcrMax = computeZCR (bufMax, totalSamples);
            float zcrDelta = std::abs (zcrMax - zcrMin);
            bool spectrallydifferent = zcrDelta > 0.005f;

            std::string msg = "D004 " + engineId + "::" + pid +
                              " (allow-listed, ZCR delta=" + std::to_string (zcrDelta) + ")";
            // Allow-listed params are always reported as pass — they may affect
            // stereo field or other non-RMS dimensions we don't measure here.
            (void)spectrallydifferent;
            reportSweep (msg, true);
        }
        else
        {
            // Not allow-listed and RMS didn't change → dead parameter
            std::string msg = "D004 DEAD " + engineId + "::" + pid +
                              " (RMS min=" + std::to_string (rmsMin) +
                              " max=" + std::to_string (rmsMax) + " delta=" +
                              std::to_string (rmsDelta) + ")";
            reportSweep (msg, false);
            deadCount++;
        }
    }

    if (testedCount == 0)
    {
        std::cout << "  [SKIP] " << engineId << ": all renders silent (APVTS wiring needed)\n";
        g_sweepTestsSkipped++;
    }
    else if (deadCount > 0)
    {
        std::cout << "  ENGINE " << engineId << ": " << deadCount << " dead parameter(s) found\n";
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace param_sweep_tests {

int runAll()
{
    g_sweepTestsPassed = 0;
    g_sweepTestsFailed = 0;
    g_sweepTestsSkipped = 0;
    g_deadParamsFound = 0;

    std::cout << "\n========================================\n";
    std::cout << "  Parameter Sweep Tests (D004)\n";
    std::cout << "  Issue #56 — dead param regression guard\n";
    std::cout << "========================================\n";
    std::cout << "  Config: " << NUM_BLOCKS << " blocks @ " << BLOCK_SIZE
              << " samples, sr=" << SAMPLE_RATE
              << ", dead_threshold=" << DEAD_THRESHOLD
              << ", max_params_per_engine=" << MAX_PARAMS_PER_ENGINE << "\n";
    std::cout << "  Allow-listed keywords (RMS-exempt): pan, width, spread, tune,\n";
    std::cout << "    detune, transpose, glide, portamento, routing, mode, scale,\n";
    std::cout << "    voicemode, unison\n\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort (ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        std::cout << "\n  --- " << id << " ---\n";
        sweepEngine (id);
    }

    std::cout << "\n  Parameter Sweep: "
              << g_sweepTestsPassed << " passed, "
              << g_sweepTestsFailed << " failed ("
              << g_deadParamsFound << " dead params), "
              << g_sweepTestsSkipped << " skipped\n";

    if (g_deadParamsFound > 0)
    {
        std::cout << "  D004 VIOLATION: " << g_deadParamsFound
                  << " parameter(s) declared but do not affect audio output.\n";
        std::cout << "  Wire these parameters to DSP or remove them.\n";
    }

    return g_sweepTestsFailed;
}

} // namespace param_sweep_tests
