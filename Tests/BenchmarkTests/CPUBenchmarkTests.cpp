/*
    XOceanus CPU Performance Benchmark Tests (#97)
    ================================================
    Measures audio rendering throughput for representative engines.
    No external benchmark library — uses std::chrono only.

    Strategy
    --------
    For each engine under test:
      1. prepare() at 44100 Hz / 512-sample block size.
      2. Fire a MIDI note-on so the engine is actually synthesising.
      3. Render enough blocks to fill exactly 1 second of audio.
      4. Measure wall-clock time with std::chrono::steady_clock.
      5. Compute realtime ratio = 1.0 / elapsed_seconds.
      6. PASS if realtime ratio >= REALTIME_THRESHOLD (2x).

    Engines selected
    ----------------
    - ONSET      (OnsetEngine)    — lightest: percussive, few voices, short tails
    - OBRIX      (ObrixEngine)    — flagship: modular brick synthesis, 81 params
    - OUROBOROS  (OuroborosEngine) — heaviest: chaotic attractor, 8-voice poly

    These three span the CPU cost spectrum and catch regressions at both ends.

    Pass threshold: 2x realtime (generous; real-world target is >> 10x on M1).
    If a machine cannot do 2x it is either absurdly slow or there is a DSP bug.

    No test framework — assert-based with descriptive console output,
    matching the existing XOceanus test style.
*/

#include "CPUBenchmarkTests.h"

#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"

#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Obrix/ObrixEngine.h"
#include "Engines/Ouroboros/OuroborosEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace xoceanus;

//==============================================================================
// Engine registration — idempotent; safe to call if already registered by
// another test translation unit linked into the same binary.
//==============================================================================

static bool reg_bench_Onset     = EngineRegistry::instance().registerEngine(
    "Onset",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OnsetEngine>(); });
static bool reg_bench_Obrix     = EngineRegistry::instance().registerEngine(
    "Obrix",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObrixEngine>(); });
static bool reg_bench_Ouroboros = EngineRegistry::instance().registerEngine(
    "Ouroboros", []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OuroborosEngine>(); });

//==============================================================================
// Benchmark constants
//==============================================================================

static constexpr double BENCH_SAMPLE_RATE      = 44100.0;
static constexpr int    BENCH_BLOCK_SIZE        = 512;
// Number of blocks that fill exactly 1 second of audio at 44100/512.
// 44100 / 512 = 86.13... — use ceil so we render at least 1 second.
static constexpr int    BENCH_BLOCKS_PER_SECOND = 87; // ceil(44100 / 512)
static constexpr double REALTIME_THRESHOLD      = 2.0; // minimum acceptable x-realtime

//==============================================================================
// Minimal AudioProcessor stub — required to construct an APVTS.
// Identical pattern to SweepTestProcessor in ParameterSweepTests.cpp.
//==============================================================================

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

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_benchPassed = 0;
static int g_benchFailed = 0;

static void reportBench (const std::string& name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_benchPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_benchFailed++;
    }
}

//==============================================================================
// Core benchmark: render 1 second of audio, return elapsed ms and RT ratio.
//==============================================================================

struct BenchResult
{
    double elapsedMs    = 0.0;
    double realtimeRatio = 0.0;
    bool   ok           = false; // false if engine creation or prepare() failed
};

static BenchResult benchmarkEngine (const std::string& engineId)
{
    BenchResult result;

    auto engine = EngineRegistry::instance().createEngine (engineId);
    if (!engine)
    {
        std::cout << "  [SKIP] " << engineId << ": engine creation failed\n";
        return result;
    }

    // Wire parameters via a minimal APVTS so attachParameters() succeeds.
    BenchTestProcessor proc;
    juce::AudioProcessorValueTreeState apvts (
        proc, nullptr, "PARAMS", engine->createParameterLayout());
    engine->attachParameters (apvts);

    engine->prepare (BENCH_SAMPLE_RATE, BENCH_BLOCK_SIZE);
    engine->reset();

    juce::AudioBuffer<float> blockBuf (2, BENCH_BLOCK_SIZE);

    // Pre-build MIDI buffers: note-on for block 0, empty for all subsequent blocks.
    juce::MidiBuffer noteOnMidi;
    noteOnMidi.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<uint8_t>(100)), 0);
    juce::MidiBuffer emptyMidi;

    // Wake the SilenceGate so it doesn't short-circuit DSP during the benchmark.
    engine->wakeSilenceGate();

    // --- timed region ---
    const auto startTime = std::chrono::steady_clock::now();

    for (int block = 0; block < BENCH_BLOCKS_PER_SECOND; ++block)
    {
        blockBuf.clear();
        juce::MidiBuffer& midi = (block == 0) ? noteOnMidi : emptyMidi;
        engine->renderBlock (blockBuf, midi, BENCH_BLOCK_SIZE);
    }

    const auto endTime = std::chrono::steady_clock::now();
    // --- end timed region ---

    const double elapsedSec =
        std::chrono::duration<double>(endTime - startTime).count();

    result.elapsedMs     = elapsedSec * 1000.0;
    result.realtimeRatio = (elapsedSec > 0.0) ? (1.0 / elapsedSec) : 0.0;
    result.ok            = true;

    return result;
}

//==============================================================================
// Individual engine benchmark tests
//==============================================================================

static void benchmarkOnset()
{
    std::cout << "\n--- ONSET (lightest) ---\n";
    const auto res = benchmarkEngine ("Onset");
    if (!res.ok) return;

    std::cout << "  Render time : " << res.elapsedMs << " ms\n";
    std::cout << "  RT ratio    : " << res.realtimeRatio << "x realtime\n";

    reportBench ("ONSET renders >= 2x realtime", res.realtimeRatio >= REALTIME_THRESHOLD);
}

static void benchmarkObrix()
{
    std::cout << "\n--- OBRIX (flagship) ---\n";
    const auto res = benchmarkEngine ("Obrix");
    if (!res.ok) return;

    std::cout << "  Render time : " << res.elapsedMs << " ms\n";
    std::cout << "  RT ratio    : " << res.realtimeRatio << "x realtime\n";

    reportBench ("OBRIX renders >= 2x realtime", res.realtimeRatio >= REALTIME_THRESHOLD);
}

static void benchmarkOuroboros()
{
    std::cout << "\n--- OUROBOROS (heaviest) ---\n";
    const auto res = benchmarkEngine ("Ouroboros");
    if (!res.ok) return;

    std::cout << "  Render time : " << res.elapsedMs << " ms\n";
    std::cout << "  RT ratio    : " << res.realtimeRatio << "x realtime\n";

    reportBench ("OUROBOROS renders >= 2x realtime", res.realtimeRatio >= REALTIME_THRESHOLD);
}

//==============================================================================
// Public entry point
//==============================================================================

namespace cpu_benchmark_tests {

int runAll()
{
    std::cout << "\n========================================\n";
    std::cout << "  CPU Performance Benchmark Tests (#97)\n";
    std::cout << "  Sample rate : " << BENCH_SAMPLE_RATE << " Hz\n";
    std::cout << "  Block size  : " << BENCH_BLOCK_SIZE << " samples\n";
    std::cout << "  Duration    : 1 second of audio per engine\n";
    std::cout << "  Threshold   : " << REALTIME_THRESHOLD << "x realtime\n";
    std::cout << "========================================\n";

    g_benchPassed = 0;
    g_benchFailed = 0;

    benchmarkOnset();
    benchmarkObrix();
    benchmarkOuroboros();

    std::cout << "\n--- Benchmark Summary ---\n";
    std::cout << "  Passed: " << g_benchPassed << "\n";
    std::cout << "  Failed: " << g_benchFailed << "\n";

    return g_benchFailed;
}

} // namespace cpu_benchmark_tests
