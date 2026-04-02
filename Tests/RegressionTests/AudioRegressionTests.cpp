/*
    XOceanus Audio Regression / Golden-File Tests (#103)
    ======================================================
    Renders a short buffer from each key engine and verifies a simple audio
    fingerprint (RMS + peak) against known-good baseline values.

    Purpose
    -------
    Catches *unintentional* DSP changes. If someone modifies an engine and the
    audio output shifts, this test fails, forcing a deliberate decision:
      (a) Fix the regression — restore the old sound.
      (b) Update the golden values — acknowledge the intentional change.

    Engines under test
    ------------------
    - OBRIX      (flagship, 81 params)
    - ONSET      (lightest percussive engine)
    - OUROBOROS  (chaotic attractor, 8-voice poly)

    Render parameters
    -----------------
    - Sample rate : 44100 Hz
    - Block size  : 256 samples  (single block — deterministic output)
    - Note        : Middle C (MIDI 60), velocity 100
    - Duration    : 256 samples only (the first rendered block)

    Fingerprint
    -----------
    - RMS   : sqrt(mean(sample^2))  over all samples in both channels
    - Peak  : max(abs(sample))      over all samples in both channels

    Tolerance : ±5 % (relative) on both metrics.

    First-run bootstrap
    -------------------
    When no baseline exists yet the golden values below are set to -1.0.
    The test prints the actual values and FAILS, so you can paste the
    printed numbers in as the new baselines.  This is intentional — the
    first run establishes the ground truth.
*/

#include "AudioRegressionTests.h"

#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"

#include "Engines/Obrix/ObrixEngine.h"
#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Ouroboros/OuroborosEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <iostream>
#include <string>

using namespace xoceanus;

//==============================================================================
// Engine registration — idempotent; safe if another TU already registered them.
//==============================================================================

static bool reg_reg_Obrix     = EngineRegistry::instance().registerEngine(
    "Obrix",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObrixEngine>(); });
static bool reg_reg_Onset     = EngineRegistry::instance().registerEngine(
    "Onset",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OnsetEngine>(); });
static bool reg_reg_Ouroboros = EngineRegistry::instance().registerEngine(
    "Ouroboros", []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OuroborosEngine>(); });

//==============================================================================
// Render constants
//==============================================================================

static constexpr double REG_SAMPLE_RATE  = 44100.0;
static constexpr int    REG_BLOCK_SIZE   = 256;
static constexpr int    REG_NOTE         = 60;   // middle C
static constexpr int    REG_VELOCITY     = 100;
static constexpr double REG_TOLERANCE    = 0.05; // ±5 %

//==============================================================================
// Golden baselines
// Set to -1.0 to bootstrap.  On first run the test prints actual values;
// paste them here and commit to lock in the baseline.
//==============================================================================

struct GoldenValues
{
    double rms;
    double peak;
};

static constexpr GoldenValues GOLDEN_OBRIX     = { -1.0, -1.0 };
static constexpr GoldenValues GOLDEN_ONSET     = { -1.0, -1.0 };
static constexpr GoldenValues GOLDEN_OUROBOROS = { -1.0, -1.0 };

//==============================================================================
// Minimal AudioProcessor stub (same pattern as BenchTestProcessor).
//==============================================================================

struct RegTestProcessor : juce::AudioProcessor
{
    const juce::String getName() const override               { return "RegTestProcessor"; }
    void prepareToPlay (double, int) override                 {}
    void releaseResources() override                          {}
    void processBlock (juce::AudioBuffer<float>&,
                       juce::MidiBuffer&) override            {}
    double getTailLengthSeconds() const override              { return 0.0; }
    bool acceptsMidi() const override                         { return false; }
    bool producesMidi() const override                        { return false; }
    juce::AudioProcessorEditor* createEditor() override       { return nullptr; }
    bool hasEditor() const override                           { return false; }
    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override    {}
    void setStateInformation (const void*, int) override      {}
};

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_regPassed = 0;
static int g_regFailed = 0;

static void reportReg (const std::string& name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        ++g_regPassed;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        ++g_regFailed;
    }
}

//==============================================================================
// Fingerprint helpers
//==============================================================================

struct Fingerprint { double rms; double peak; };

static Fingerprint computeFingerprint (const juce::AudioBuffer<float>& buf)
{
    double sumSq = 0.0;
    double peak  = 0.0;
    const int totalSamples = buf.getNumChannels() * buf.getNumSamples();

    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer (ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const double s = static_cast<double> (data[i]);
            sumSq += s * s;
            if (std::abs (s) > peak) peak = std::abs (s);
        }
    }

    const double rms = (totalSamples > 0) ? std::sqrt (sumSq / totalSamples) : 0.0;
    return { rms, peak };
}

static bool withinTolerance (double actual, double expected)
{
    // If expected is still the bootstrap sentinel (-1) treat as "not yet set".
    if (expected < 0.0) return false;
    if (expected == 0.0) return std::abs (actual) < 1e-9;
    return std::abs (actual - expected) / expected <= REG_TOLERANCE;
}

//==============================================================================
// Core render + compare
//==============================================================================

static void runEngineRegression (const std::string& engineId,
                                 const GoldenValues& golden)
{
    std::cout << "\n--- " << engineId << " ---\n";

    auto engine = EngineRegistry::instance().createEngine (engineId);
    if (!engine)
    {
        std::cout << "  [SKIP] engine creation failed\n";
        return;
    }

    RegTestProcessor proc;
    juce::AudioProcessorValueTreeState apvts (
        proc, nullptr, "PARAMS", engine->createParameterLayout());
    engine->attachParameters (apvts);

    engine->prepare (REG_SAMPLE_RATE, REG_BLOCK_SIZE);
    engine->reset();
    engine->wakeSilenceGate();

    juce::AudioBuffer<float> buf (2, REG_BLOCK_SIZE);
    buf.clear();

    juce::MidiBuffer midi;
    midi.addEvent (
        juce::MidiMessage::noteOn (1, REG_NOTE, static_cast<uint8_t> (REG_VELOCITY)), 0);

    engine->renderBlock (buf, midi, REG_BLOCK_SIZE);

    const Fingerprint fp = computeFingerprint (buf);

    std::cout << "  Actual  RMS  : " << fp.rms  << "\n";
    std::cout << "  Actual  Peak : " << fp.peak << "\n";
    std::cout << "  Golden  RMS  : " << golden.rms  << "\n";
    std::cout << "  Golden  Peak : " << golden.peak << "\n";

    const bool bootstrapping = (golden.rms < 0.0 || golden.peak < 0.0);

    if (bootstrapping)
    {
        std::cout << "  [BOOTSTRAP] No baseline yet — paste these values into "
                     "AudioRegressionTests.cpp GOLDEN_" << engineId << ":\n";
        std::cout << "    { " << fp.rms << ", " << fp.peak << " }\n";
        reportReg (engineId + " golden baseline (needs bootstrap)", false);
        return;
    }

    const bool rmsOk  = withinTolerance (fp.rms,  golden.rms);
    const bool peakOk = withinTolerance (fp.peak, golden.peak);

    if (!rmsOk)
        std::cout << "  RMS delta : " << std::abs (fp.rms - golden.rms) / golden.rms * 100.0 << "% (limit 5%)\n";
    if (!peakOk)
        std::cout << "  Peak delta: " << std::abs (fp.peak - golden.peak) / golden.peak * 100.0 << "% (limit 5%)\n";

    reportReg (engineId + " RMS within 5%",  rmsOk);
    reportReg (engineId + " Peak within 5%", peakOk);
}

//==============================================================================
// Public entry point
//==============================================================================

namespace audio_regression_tests {

int runAll()
{
    std::cout << "\n========================================\n";
    std::cout << "  Audio Regression Tests (#103)\n";
    std::cout << "  Sample rate : " << REG_SAMPLE_RATE << " Hz\n";
    std::cout << "  Block size  : " << REG_BLOCK_SIZE << " samples\n";
    std::cout << "  Note        : MIDI " << REG_NOTE << " (middle C), vel " << REG_VELOCITY << "\n";
    std::cout << "  Tolerance   : " << (REG_TOLERANCE * 100.0) << "%\n";
    std::cout << "  Baselines   : -1.0 means bootstrap needed (test will print values)\n";
    std::cout << "========================================\n";

    g_regPassed = 0;
    g_regFailed = 0;

    runEngineRegression ("Obrix",     GOLDEN_OBRIX);
    runEngineRegression ("Onset",     GOLDEN_ONSET);
    runEngineRegression ("Ouroboros", GOLDEN_OUROBOROS);

    std::cout << "\n--- Regression Summary ---\n";
    std::cout << "  Passed: " << g_regPassed << "\n";
    std::cout << "  Failed: " << g_regFailed << "\n";

    return g_regFailed;
}

} // namespace audio_regression_tests
