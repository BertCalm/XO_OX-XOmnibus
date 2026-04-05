/*
    XOceanus Audio Regression / Golden-File Tests (#103)
    ======================================================
    Renders a short buffer from each key engine and verifies a simple audio
    fingerprint (RMS + peak) against known-good baseline values.

    Golden baselines are set to -1.0 to bootstrap. On first run the test
    prints actual values and fails; paste them in to lock the baseline.
    Migrated to Catch2 v3: issue #81
*/

#include "AudioRegressionTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"

#include "Engines/Obrix/ObrixEngine.h"
#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Ouroboros/OuroborosEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <string>

using namespace xoceanus;

static bool reg_reg_Obrix = EngineRegistry::instance().registerEngine("Obrix", []() -> std::unique_ptr<SynthEngine>
                                                                      { return std::make_unique<ObrixEngine>(); });
static bool reg_reg_Onset = EngineRegistry::instance().registerEngine("Onset", []() -> std::unique_ptr<SynthEngine>
                                                                      { return std::make_unique<OnsetEngine>(); });
static bool reg_reg_Ouroboros = EngineRegistry::instance().registerEngine(
    "Ouroboros", []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OuroborosEngine>(); });

static constexpr double REG_SAMPLE_RATE = 44100.0;
static constexpr int REG_BLOCK_SIZE = 256;
static constexpr int REG_NOTE = 60;
static constexpr int REG_VELOCITY = 100;
static constexpr double REG_TOLERANCE = 0.05; // ±5%

struct GoldenValues
{
    double rms;
    double peak;
};

// Golden baselines captured 2026-04-05 on macOS (arm64, Release, 44100 Hz, 256 samples).
static constexpr GoldenValues GOLDEN_OBRIX = {0.0797815, 0.166613};
static constexpr GoldenValues GOLDEN_ONSET = {0.0, 0.0};          // percussion — silent without velocity trigger in test harness
static constexpr GoldenValues GOLDEN_OUROBOROS = {0.281754, 0.572705};

struct RegTestProcessor : juce::AudioProcessor
{
    const juce::String getName() const override { return "RegTestProcessor"; }
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

struct Fingerprint
{
    double rms;
    double peak;
};

static Fingerprint computeFingerprint(const juce::AudioBuffer<float>& buf)
{
    double sumSq = 0.0, peak = 0.0;
    const int total = buf.getNumChannels() * buf.getNumSamples();
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const double s = static_cast<double>(data[i]);
            sumSq += s * s;
            if (std::abs(s) > peak)
                peak = std::abs(s);
        }
    }
    return {(total > 0) ? std::sqrt(sumSq / total) : 0.0, peak};
}

static bool withinTolerance(double actual, double expected)
{
    if (expected < 0.0)
        return false;
    if (expected == 0.0)
        return std::abs(actual) < 1e-9;
    return std::abs(actual - expected) / expected <= REG_TOLERANCE;
}

static Fingerprint renderEngineFingerprint(const std::string& engineId)
{
    auto engine = EngineRegistry::instance().createEngine(engineId);
    if (!engine)
        return {-1.0, -1.0};

    RegTestProcessor proc;
    juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMS", engine->createParameterLayout());
    engine->attachParameters(apvts);

    engine->prepare(REG_SAMPLE_RATE, REG_BLOCK_SIZE);
    engine->reset();
    engine->wakeSilenceGate();

    juce::AudioBuffer<float> buf(2, REG_BLOCK_SIZE);
    buf.clear();

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, REG_NOTE, static_cast<uint8_t>(REG_VELOCITY)), 0);

    engine->renderBlock(buf, midi, REG_BLOCK_SIZE);
    return computeFingerprint(buf);
}

TEST_CASE("Audio Regression - OBRIX fingerprint within 5% of golden", "[regression][obrix]")
{
    const auto fp = renderEngineFingerprint("Obrix");
    REQUIRE(fp.rms >= 0.0); // engine must exist

    INFO("Actual RMS=" << fp.rms << " Peak=" << fp.peak << " | Golden RMS=" << GOLDEN_OBRIX.rms
                       << " Peak=" << GOLDEN_OBRIX.peak);

    if (GOLDEN_OBRIX.rms < 0.0)
    {
        FAIL("BOOTSTRAP NEEDED — paste into GOLDEN_OBRIX: { " << fp.rms << ", " << fp.peak << " }");
    }
    CHECK(withinTolerance(fp.rms, GOLDEN_OBRIX.rms));
    CHECK(withinTolerance(fp.peak, GOLDEN_OBRIX.peak));
}

TEST_CASE("Audio Regression - ONSET fingerprint within 5% of golden", "[regression][onset]")
{
    const auto fp = renderEngineFingerprint("Onset");
    REQUIRE(fp.rms >= 0.0);

    INFO("Actual RMS=" << fp.rms << " Peak=" << fp.peak << " | Golden RMS=" << GOLDEN_ONSET.rms
                       << " Peak=" << GOLDEN_ONSET.peak);

    if (GOLDEN_ONSET.rms < 0.0)
    {
        FAIL("BOOTSTRAP NEEDED — paste into GOLDEN_ONSET: { " << fp.rms << ", " << fp.peak << " }");
    }
    CHECK(withinTolerance(fp.rms, GOLDEN_ONSET.rms));
    CHECK(withinTolerance(fp.peak, GOLDEN_ONSET.peak));
}

TEST_CASE("Audio Regression - OUROBOROS fingerprint within 5% of golden", "[regression][ouroboros]")
{
    const auto fp = renderEngineFingerprint("Ouroboros");
    REQUIRE(fp.rms >= 0.0);

    INFO("Actual RMS=" << fp.rms << " Peak=" << fp.peak << " | Golden RMS=" << GOLDEN_OUROBOROS.rms
                       << " Peak=" << GOLDEN_OUROBOROS.peak);

    if (GOLDEN_OUROBOROS.rms < 0.0)
    {
        FAIL("BOOTSTRAP NEEDED — paste into GOLDEN_OUROBOROS: { " << fp.rms << ", " << fp.peak << " }");
    }
    CHECK(withinTolerance(fp.rms, GOLDEN_OUROBOROS.rms));
    CHECK(withinTolerance(fp.peak, GOLDEN_OUROBOROS.peak));
}

