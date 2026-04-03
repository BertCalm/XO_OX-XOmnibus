/*
    XOceanus CPU Performance Benchmark Tests (#97)
    ================================================
    Measures audio rendering throughput for representative engines.
    Uses std::chrono only — no external benchmark library.

    Pass threshold: 2x realtime (generous; real-world target is >> 10x on M1).
    Migrated to Catch2 v3: issue #81
*/

#include "CPUBenchmarkTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"

#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Obrix/ObrixEngine.h"
#include "Engines/Ouroboros/OuroborosEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <chrono>
#include <cmath>
#include <memory>
#include <string>

using namespace xoceanus;

// Engine registration — idempotent; safe to call if already registered by
// another test translation unit linked into the same binary.
static bool reg_bench_Onset     = EngineRegistry::instance().registerEngine(
    "Onset",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OnsetEngine>(); });
static bool reg_bench_Obrix     = EngineRegistry::instance().registerEngine(
    "Obrix",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObrixEngine>(); });
static bool reg_bench_Ouroboros = EngineRegistry::instance().registerEngine(
    "Ouroboros", []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OuroborosEngine>(); });

static constexpr double BENCH_SAMPLE_RATE      = 44100.0;
static constexpr int    BENCH_BLOCK_SIZE        = 512;
static constexpr int    BENCH_BLOCKS_PER_SECOND = 87; // ceil(44100 / 512)
static constexpr double REALTIME_THRESHOLD      = 2.0;

struct BenchTestProcessor : juce::AudioProcessor
{
    const juce::String getName() const override              { return "BenchTestProcessor"; }
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

struct BenchResult
{
    double elapsedMs     = 0.0;
    double realtimeRatio = 0.0;
    bool   ok            = false;
};

static BenchResult benchmarkEngine(const std::string& engineId)
{
    BenchResult result;

    auto engine = EngineRegistry::instance().createEngine(engineId);
    if (!engine) return result;

    BenchTestProcessor proc;
    juce::AudioProcessorValueTreeState apvts(
        proc, nullptr, "PARAMS", engine->createParameterLayout());
    engine->attachParameters(apvts);

    engine->prepare(BENCH_SAMPLE_RATE, BENCH_BLOCK_SIZE);
    engine->reset();

    juce::AudioBuffer<float> blockBuf(2, BENCH_BLOCK_SIZE);

    juce::MidiBuffer noteOnMidi;
    noteOnMidi.addEvent(juce::MidiMessage::noteOn(1, 60, static_cast<uint8_t>(100)), 0);
    juce::MidiBuffer emptyMidi;

    engine->wakeSilenceGate();

    const auto startTime = std::chrono::steady_clock::now();
    for (int block = 0; block < BENCH_BLOCKS_PER_SECOND; ++block)
    {
        blockBuf.clear();
        juce::MidiBuffer& midi = (block == 0) ? noteOnMidi : emptyMidi;
        engine->renderBlock(blockBuf, midi, BENCH_BLOCK_SIZE);
    }
    const auto endTime = std::chrono::steady_clock::now();

    const double elapsedSec =
        std::chrono::duration<double>(endTime - startTime).count();

    result.elapsedMs     = elapsedSec * 1000.0;
    result.realtimeRatio = (elapsedSec > 0.0) ? (1.0 / elapsedSec) : 0.0;
    result.ok            = true;

    return result;
}

TEST_CASE("CPU Benchmark - ONSET renders >= 2x realtime", "[benchmark][onset]")
{
    const auto res = benchmarkEngine("Onset");
    REQUIRE(res.ok);
    INFO("Render time: " << res.elapsedMs << " ms, RT ratio: " << res.realtimeRatio << "x");
    CHECK(res.realtimeRatio >= REALTIME_THRESHOLD);
}

TEST_CASE("CPU Benchmark - OBRIX renders >= 2x realtime", "[benchmark][obrix]")
{
    const auto res = benchmarkEngine("Obrix");
    REQUIRE(res.ok);
    INFO("Render time: " << res.elapsedMs << " ms, RT ratio: " << res.realtimeRatio << "x");
    CHECK(res.realtimeRatio >= REALTIME_THRESHOLD);
}

TEST_CASE("CPU Benchmark - OUROBOROS renders >= 2x realtime", "[benchmark][ouroboros]")
{
    const auto res = benchmarkEngine("Ouroboros");
    REQUIRE(res.ok);
    INFO("Render time: " << res.elapsedMs << " ms, RT ratio: " << res.realtimeRatio << "x");
    CHECK(res.realtimeRatio >= REALTIME_THRESHOLD);
}

// Backward-compat shim
namespace cpu_benchmark_tests {
int runAll() { return 0; }
} // namespace cpu_benchmark_tests
