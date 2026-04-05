/*
    XOceanus DSP Stability Tests
    =============================
    Tests for FastMath, CytomicSVF, PolyBLEP, and per-engine rendering stability.
    Migrated to Catch2 v3: issue #81
*/

#include "DSPStabilityTests.h"

#include <catch2/catch_test_macros.hpp>

#include "DSP/FastMath.h"
#include "DSP/CytomicSVF.h"
#include "DSP/PolyBLEP.h"
#include "Core/EngineRegistry.h"
#include "Core/SynthEngine.h"

// Include all engine headers so their static registrations execute in test binary
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

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>
#include <set>

using namespace xoceanus;

//==============================================================================
// Engine registration for the test binary.
// The main plugin registers engines in XOceanusProcessor.cpp via static init.
// We must register them here for the standalone test executable.
//==============================================================================

static bool reg_OddfeliX = EngineRegistry::instance().registerEngine("OddfeliX", []() -> std::unique_ptr<SynthEngine>
                                                                     { return std::make_unique<SnapEngine>(); });
static bool reg_OddOscar = EngineRegistry::instance().registerEngine("OddOscar", []() -> std::unique_ptr<SynthEngine>
                                                                     { return std::make_unique<MorphEngine>(); });
static bool reg_Overdub = EngineRegistry::instance().registerEngine("Overdub", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<DubEngine>(); });
static bool reg_Odyssey = EngineRegistry::instance().registerEngine("Odyssey", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<DriftEngine>(); });
static bool reg_Oblong = EngineRegistry::instance().registerEngine("Oblong", []() -> std::unique_ptr<SynthEngine>
                                                                   { return std::make_unique<BobEngine>(); });
static bool reg_Obese = EngineRegistry::instance().registerEngine("Obese", []() -> std::unique_ptr<SynthEngine>
                                                                  { return std::make_unique<FatEngine>(); });
static bool reg_Onset = EngineRegistry::instance().registerEngine("Onset", []() -> std::unique_ptr<SynthEngine>
                                                                  { return std::make_unique<OnsetEngine>(); });
static bool reg_Overworld = EngineRegistry::instance().registerEngine("Overworld", []() -> std::unique_ptr<SynthEngine>
                                                                      { return std::make_unique<OverworldEngine>(); });
static bool reg_Opal = EngineRegistry::instance().registerEngine("Opal", []() -> std::unique_ptr<SynthEngine>
                                                                 { return std::make_unique<OpalEngine>(); });
static bool reg_Bite = EngineRegistry::instance().registerEngine("Bite", []() -> std::unique_ptr<SynthEngine>
                                                                 { return std::make_unique<BiteEngine>(); });
static bool reg_Organon = EngineRegistry::instance().registerEngine("Organon", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OrganonEngine>(); });
static bool reg_Ocelot = EngineRegistry::instance().registerEngine(
    "Ocelot", []() -> std::unique_ptr<SynthEngine> { return std::make_unique<xocelot::OcelotEngine>(); });
static bool reg_Ouroboros = EngineRegistry::instance().registerEngine("Ouroboros", []() -> std::unique_ptr<SynthEngine>
                                                                      { return std::make_unique<OuroborosEngine>(); });
static bool reg_Obsidian = EngineRegistry::instance().registerEngine("Obsidian", []() -> std::unique_ptr<SynthEngine>
                                                                     { return std::make_unique<ObsidianEngine>(); });
static bool reg_Origami = EngineRegistry::instance().registerEngine("Origami", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OrigamiEngine>(); });
static bool reg_Oracle = EngineRegistry::instance().registerEngine("Oracle", []() -> std::unique_ptr<SynthEngine>
                                                                   { return std::make_unique<OracleEngine>(); });
static bool reg_Obscura = EngineRegistry::instance().registerEngine("Obscura", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<ObscuraEngine>(); });
static bool reg_Oceanic = EngineRegistry::instance().registerEngine("Oceanic", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OceanicEngine>(); });
static bool reg_Optic = EngineRegistry::instance().registerEngine("Optic", []() -> std::unique_ptr<SynthEngine>
                                                                  { return std::make_unique<OpticEngine>(); });
static bool reg_Oblique = EngineRegistry::instance().registerEngine("Oblique", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<ObliqueEngine>(); });
static bool reg_Orbital = EngineRegistry::instance().registerEngine("Orbital", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OrbitalEngine>(); });
static bool reg_Osprey = EngineRegistry::instance().registerEngine("Osprey", []() -> std::unique_ptr<SynthEngine>
                                                                   { return std::make_unique<OspreyEngine>(); });
static bool reg_Osteria = EngineRegistry::instance().registerEngine("Osteria", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OsteriaEngine>(); });
static bool reg_Owlfish = EngineRegistry::instance().registerEngine(
    "Owlfish", []() -> std::unique_ptr<SynthEngine> { return std::make_unique<xowlfish::OwlfishEngine>(); });
static bool reg_Ohm = EngineRegistry::instance().registerEngine("Ohm", []() -> std::unique_ptr<SynthEngine>
                                                                { return std::make_unique<OhmEngine>(); });
static bool reg_Orphica = EngineRegistry::instance().registerEngine("Orphica", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OrphicaEngine>(); });
static bool reg_Obbligato = EngineRegistry::instance().registerEngine("Obbligato", []() -> std::unique_ptr<SynthEngine>
                                                                      { return std::make_unique<ObbligatoEngine>(); });
static bool reg_Ottoni = EngineRegistry::instance().registerEngine("Ottoni", []() -> std::unique_ptr<SynthEngine>
                                                                   { return std::make_unique<OttoniEngine>(); });
static bool reg_Ole = EngineRegistry::instance().registerEngine("Ole", []() -> std::unique_ptr<SynthEngine>
                                                                { return std::make_unique<OleEngine>(); });
static bool reg_XOverlap = EngineRegistry::instance().registerEngine("XOverlap", []() -> std::unique_ptr<SynthEngine>
                                                                     { return std::make_unique<XOverlapEngine>(); });
static bool reg_XOutwit = EngineRegistry::instance().registerEngine("XOutwit", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<XOutwitEngine>(); });
static bool reg_Ombre = EngineRegistry::instance().registerEngine("Ombre", []() -> std::unique_ptr<SynthEngine>
                                                                  { return std::make_unique<OmbreEngine>(); });
static bool reg_Orca = EngineRegistry::instance().registerEngine("Orca", []() -> std::unique_ptr<SynthEngine>
                                                                 { return std::make_unique<OrcaEngine>(); });
static bool reg_Octopus = EngineRegistry::instance().registerEngine("Octopus", []() -> std::unique_ptr<SynthEngine>
                                                                    { return std::make_unique<OctopusEngine>(); });

//==============================================================================
// Helpers
//==============================================================================

static bool isFiniteVal(float v)
{
    return std::isfinite(v) && !std::isnan(v);
}

static bool isDenormal(float v)
{
    return std::fpclassify(v) == FP_SUBNORMAL;
}

//==============================================================================
// FastMath tests
//==============================================================================

TEST_CASE("FastMath - flushDenormal returns 0 for denormal input", "[dsp][fastmath]")
{
    float denorm = std::numeric_limits<float>::denorm_min();
    CHECK(flushDenormal(denorm) == 0.0f);
}

TEST_CASE("FastMath - flushDenormal preserves normal value 1.0", "[dsp][fastmath]")
{
    CHECK(flushDenormal(1.0f) == 1.0f);
}

TEST_CASE("FastMath - flushDenormal preserves zero", "[dsp][fastmath]")
{
    CHECK(flushDenormal(0.0f) == 0.0f);
}

TEST_CASE("FastMath - flushDenormal preserves negative normal", "[dsp][fastmath]")
{
    CHECK(flushDenormal(-42.5f) == -42.5f);
}

// fastExp uses Schraudolph's IEEE 754 bit-manipulation method.
// Measured worst-case relative error is ~6.2% (peaks near x=1.0).
// 7% tolerance gives a small margin above the measured peak.
// Sufficient precision for envelope curves and gain automation.
TEST_CASE("FastMath - fastExp within 7% of std::exp for [-10,10]", "[dsp][fastmath]")
{
    bool withinTolerance = true;
    for (float x = -10.0f; x <= 10.0f; x += 0.1f)
    {
        float fast = fastExp(x);
        float ref = std::exp(x);
        if (ref > 1e-6f)
        {
            float relErr = std::abs(fast - ref) / ref;
            if (relErr > 0.07f)
                withinTolerance = false;
        }
    }
    CHECK(withinTolerance);
}

// fastTanh uses a Padé rational approximant: x*(27+x²)/(27+9x²).
// Measured worst-case absolute error is ~0.0235 near x=±1.57 (the inflection region).
// 0.03 absolute tolerance gives a margin above the measured peak.
// Sufficient precision for saturation and waveshaping in audio DSP.
TEST_CASE("FastMath - fastTanh within 3% absolute of std::tanh for [-3,3]", "[dsp][fastmath]")
{
    bool withinTolerance = true;
    for (float x = -3.0f; x <= 3.0f; x += 0.05f)
    {
        if (std::abs(fastTanh(x) - std::tanh(x)) > 0.03f)
            withinTolerance = false;
    }
    CHECK(withinTolerance);
}

TEST_CASE("FastMath - fastSin within 1% of std::sin for full period", "[dsp][fastmath]")
{
    bool withinTolerance = true;
    constexpr float twoPi = 6.28318530717958647692f;
    for (float x = -twoPi; x <= twoPi; x += 0.01f)
    {
        if (std::abs(fastSin(x) - std::sin(x)) > 0.01f)
            withinTolerance = false;
    }
    CHECK(withinTolerance);
}

TEST_CASE("FastMath - midiToFreq(69) = 440 Hz", "[dsp][fastmath]")
{
    CHECK(std::abs(midiToFreq(69) - 440.0f) < 0.01f);
}

TEST_CASE("FastMath - dbToGain(0) = 1.0", "[dsp][fastmath]")
{
    CHECK(std::abs(dbToGain(0.0f) - 1.0f) < 0.001f);
}

TEST_CASE("FastMath - dbToGain(-6) ~ 0.5", "[dsp][fastmath]")
{
    CHECK(std::abs(dbToGain(-6.0f) - 0.5012f) < 0.01f);
}

TEST_CASE("FastMath - gainToDb(1.0) = 0 dB", "[dsp][fastmath]")
{
    CHECK(std::abs(gainToDb(1.0f)) < 0.001f);
}

//==============================================================================
// CytomicSVF tests
//==============================================================================

TEST_CASE("CytomicSVF - after reset processSample(0) returns 0", "[dsp][svf]")
{
    constexpr float sampleRate = 44100.0f;
    CytomicSVF filter;
    filter.setMode(CytomicSVF::Mode::LowPass);
    filter.setCoefficients(1000.0f, 0.5f, sampleRate);
    filter.reset();
    CHECK(filter.processSample(0.0f) == 0.0f);
}

TEST_CASE("CytomicSVF - LowPass@1000Hz passes 100Hz (>0.9)", "[dsp][svf]")
{
    constexpr float sampleRate = 44100.0f;
    CytomicSVF filter;
    filter.setMode(CytomicSVF::Mode::LowPass);
    filter.setCoefficients(1000.0f, 0.0f, sampleRate);
    filter.reset();

    constexpr float twoPi = 6.28318530717958647692f;
    float maxOut = 0.0f;
    for (int i = 0; i < 5000; ++i)
    {
        float in = std::sin(twoPi * 100.0f * static_cast<float>(i) / sampleRate);
        float out = filter.processSample(in);
        if (i > 2000)
            maxOut = std::max(maxOut, std::abs(out));
    }
    CHECK(maxOut > 0.9f);
}

TEST_CASE("CytomicSVF - LowPass@1000Hz attenuates 10kHz (<0.1)", "[dsp][svf]")
{
    constexpr float sampleRate = 44100.0f;
    CytomicSVF filter;
    filter.setMode(CytomicSVF::Mode::LowPass);
    filter.setCoefficients(1000.0f, 0.0f, sampleRate);
    filter.reset();

    constexpr float twoPi = 6.28318530717958647692f;
    float maxOut = 0.0f;
    for (int i = 0; i < 5000; ++i)
    {
        float in = std::sin(twoPi * 10000.0f * static_cast<float>(i) / sampleRate);
        float out = filter.processSample(in);
        if (i > 2000)
            maxOut = std::max(maxOut, std::abs(out));
    }
    CHECK(maxOut < 0.1f);
}

TEST_CASE("CytomicSVF - BandPass@1000Hz passes 1000Hz (>0.1)", "[dsp][svf]")
{
    constexpr float sampleRate = 44100.0f;
    CytomicSVF filter;
    filter.setMode(CytomicSVF::Mode::BandPass);
    filter.setCoefficients(1000.0f, 0.5f, sampleRate);
    filter.reset();

    constexpr float twoPi = 6.28318530717958647692f;
    float maxOut = 0.0f;
    for (int i = 0; i < 5000; ++i)
    {
        float in = std::sin(twoPi * 1000.0f * static_cast<float>(i) / sampleRate);
        float out = filter.processSample(in);
        if (i > 2000)
            maxOut = std::max(maxOut, std::abs(out));
    }
    CHECK(maxOut > 0.1f);
}

TEST_CASE("CytomicSVF - no NaN/Inf after 10000 noise samples", "[dsp][svf]")
{
    constexpr float sampleRate = 44100.0f;
    CytomicSVF filter;
    filter.setMode(CytomicSVF::Mode::LowPass);
    filter.setCoefficients(5000.0f, 0.8f, sampleRate);
    filter.reset();

    juce::Random rng(42);
    bool stable = true;
    for (int i = 0; i < 10000; ++i)
    {
        float out = filter.processSample(rng.nextFloat() * 2.0f - 1.0f);
        if (!isFiniteVal(out))
        {
            stable = false;
            break;
        }
    }
    CHECK(stable);
}

TEST_CASE("CytomicSVF - self-oscillation (res=1.0) stable for 100000 samples", "[dsp][svf]")
{
    constexpr float sampleRate = 44100.0f;
    CytomicSVF filter;
    filter.setMode(CytomicSVF::Mode::LowPass);
    filter.setCoefficients(1000.0f, 1.0f, sampleRate);
    filter.reset();

    bool stable = true;
    for (int i = 0; i < 100000; ++i)
    {
        float out = filter.processSample((i == 0) ? 1.0f : 0.0f);
        if (!isFiniteVal(out) || std::abs(out) > 10.0f)
        {
            stable = false;
            break;
        }
    }
    CHECK(stable);
}

//==============================================================================
// PolyBLEP tests
//==============================================================================

TEST_CASE("PolyBLEP - all waveforms stay within [-1.1, 1.1]", "[dsp][polyblep]")
{
    constexpr float sampleRate = 44100.0f;
    PolyBLEP::Waveform waveforms[] = {PolyBLEP::Waveform::Sine, PolyBLEP::Waveform::Saw, PolyBLEP::Waveform::Square,
                                      PolyBLEP::Waveform::Triangle, PolyBLEP::Waveform::Pulse};

    for (int w = 0; w < 5; ++w)
    {
        PolyBLEP osc;
        osc.setWaveform(waveforms[w]);
        osc.setFrequency(440.0f, sampleRate);
        osc.reset();

        bool inRange = true;
        for (int i = 0; i < 10000; ++i)
        {
            float s = osc.processSample();
            if (s < -1.1f || s > 1.1f)
            {
                inRange = false;
                break;
            }
        }
        INFO("Waveform index " << w);
        CHECK(inRange);
    }
}

TEST_CASE("PolyBLEP - saw@440Hz RMS in [0.3, 0.9]", "[dsp][polyblep]")
{
    constexpr float sampleRate = 44100.0f;
    PolyBLEP osc;
    osc.setWaveform(PolyBLEP::Waveform::Saw);
    osc.setFrequency(440.0f, sampleRate);
    osc.reset();

    double sumSq = 0.0;
    constexpr int N = 44100;
    for (int i = 0; i < N; ++i)
    {
        float s = osc.processSample();
        sumSq += static_cast<double>(s) * static_cast<double>(s);
    }
    float rms = static_cast<float>(std::sqrt(sumSq / N));
    CHECK(rms >= 0.3f);
    CHECK(rms <= 0.9f);
}

TEST_CASE("PolyBLEP - square@440Hz RMS in [0.3, 1.1]", "[dsp][polyblep]")
{
    constexpr float sampleRate = 44100.0f;
    PolyBLEP osc;
    osc.setWaveform(PolyBLEP::Waveform::Square);
    osc.setFrequency(440.0f, sampleRate);
    osc.reset();

    double sumSq = 0.0;
    constexpr int N = 44100;
    for (int i = 0; i < N; ++i)
    {
        float s = osc.processSample();
        sumSq += static_cast<double>(s) * static_cast<double>(s);
    }
    float rms = static_cast<float>(std::sqrt(sumSq / N));
    CHECK(rms >= 0.3f);
    CHECK(rms <= 1.1f);
}

TEST_CASE("PolyBLEP - sine@440Hz within 5% of std::sin", "[dsp][polyblep]")
{
    constexpr float sampleRate = 44100.0f;
    constexpr float twoPi = 6.28318530717958647692f;
    PolyBLEP osc;
    osc.setWaveform(PolyBLEP::Waveform::Sine);
    osc.setFrequency(440.0f, sampleRate);
    osc.reset();

    bool withinTolerance = true;
    float phaseInc = 440.0f / sampleRate;
    float phase = 0.0f;
    for (int i = 0; i < 1000; ++i)
    {
        float oscOut = osc.processSample();
        float ref = std::sin(twoPi * phase);
        if (std::abs(oscOut - ref) > 0.05f)
            withinTolerance = false;
        phase += phaseInc;
        while (phase >= 1.0f)
            phase -= 1.0f;
    }
    CHECK(withinTolerance);
}

TEST_CASE("PolyBLEP - no NaN/Inf in any waveform after 100000 samples", "[dsp][polyblep]")
{
    constexpr float sampleRate = 44100.0f;
    PolyBLEP::Waveform waveforms[] = {PolyBLEP::Waveform::Sine, PolyBLEP::Waveform::Saw, PolyBLEP::Waveform::Square,
                                      PolyBLEP::Waveform::Triangle, PolyBLEP::Waveform::Pulse};

    bool allStable = true;
    for (int w = 0; w < 5; ++w)
    {
        PolyBLEP osc;
        osc.setWaveform(waveforms[w]);
        osc.setFrequency(440.0f, sampleRate);
        osc.reset();
        for (int i = 0; i < 100000; ++i)
        {
            if (!isFiniteVal(osc.processSample()))
            {
                allStable = false;
                break;
            }
        }
        if (!allStable)
            break;
    }
    CHECK(allStable);
}

//==============================================================================
// Engine Registry Creation tests
//==============================================================================

TEST_CASE("EngineRegistry - contains >= 34 registered engines", "[dsp][registry]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    CHECK(ids.size() >= 34);
}

TEST_CASE("EngineRegistry - all registered IDs create non-null engines", "[dsp][registry]")
{
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);
        auto engine = registry.createEngine(id);
        CHECK(engine != nullptr);
        if (engine)
        {
            CHECK(engine->getEngineId().isNotEmpty());
        }
    }
}

TEST_CASE("EngineRegistry - unknown ID returns nullptr", "[dsp][registry]")
{
    auto engine = EngineRegistry::instance().createEngine("NonExistentEngine_XYZ");
    CHECK(engine == nullptr);
}

//==============================================================================
// Engine rendering stability tests
//==============================================================================

TEST_CASE("Engine stability - prepare/reset/render all registered engines", "[dsp][engine]")
{
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);
        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

        engine->prepare(sampleRate, blockSize);
        engine->reset();

        // Empty MIDI: no NaN/Inf
        {
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();
            juce::MidiBuffer emptyMidi;
            engine->renderBlock(buffer, emptyMidi, blockSize);

            bool stable = true;
            for (int ch = 0; ch < buffer.getNumChannels() && stable; ++ch)
            {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i)
                    if (!isFiniteVal(data[i]))
                    {
                        stable = false;
                        break;
                    }
            }
            CHECK(stable);
        }

        // Note-on produces non-silent output
        // Some engines are architecturally exempt from this check — they still must
        // not crash, produce NaN/Inf, or generate denormals, but are not expected to
        // emit audible audio in a generic 512-sample note-on test:
        //   Optic   — B005: Zero-Audio Identity; visual modulation engine, no audio output by design
        //   Onset   — Percussion/transient engine; requires a specific trigger pattern, not a bare note-on
        //   Opal    — Granular engine; requires a loaded grain buffer before it can produce output
        //   Orbital — Additive synthesis; needs more than one note-on block for harmonic partials to accumulate
        static const std::set<std::string> silenceExempt = {
            "Optic",    // B005: Zero-Audio Identity — visual engine, no audio output by design
            "Onset",    // Percussion — requires specific trigger pattern not in generic test
            "Opal",     // Granular — requires loaded grain buffer
            "Orbital",  // Additive — needs sustained note time to build harmonics in 512 samples
        };

        {
            engine->reset();
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
            engine->renderBlock(buffer, midi, blockSize);

            float maxSample = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i)
                    maxSample = std::max(maxSample, std::abs(data[i]));
            }
            // Give engines a second block if first was silent (some need ramp-up)
            if (maxSample < 1e-6f)
            {
                buffer.clear();
                juce::MidiBuffer emptyMidi2;
                engine->renderBlock(buffer, emptyMidi2, blockSize);
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    const float* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < blockSize; ++i)
                        maxSample = std::max(maxSample, std::abs(data[i]));
                }
            }
            if (silenceExempt.find(id) == silenceExempt.end())
                CHECK(maxSample > 1e-6f);
        }

        // After reset, coupling samples = 0
        {
            engine->reset();
            float cL = engine->getSampleForCoupling(0, 0);
            float cR = engine->getSampleForCoupling(1, 0);
            CHECK(cL == 0.0f);
            CHECK(cR == 0.0f);
        }

        // No denormals in output after 10 blocks
        {
            engine->reset();
            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::MidiBuffer noteOnMidi;
            noteOnMidi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

            bool noDenormals = true;
            for (int block = 0; block < 10 && noDenormals; ++block)
            {
                buffer.clear();
                juce::MidiBuffer blockMidi;
                if (block == 0)
                    blockMidi = noteOnMidi;
                engine->renderBlock(buffer, blockMidi, blockSize);

                for (int ch = 0; ch < buffer.getNumChannels() && noDenormals; ++ch)
                {
                    const float* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < blockSize; ++i)
                        if (isDenormal(data[i]))
                        {
                            noDenormals = false;
                            break;
                        }
                }
            }
            CHECK(noDenormals);
        }
    }
}

//==============================================================================
// Fleet-wide denormal protection test
//==============================================================================

TEST_CASE("Fleet - no denormals in 50-block note-on/off cycle", "[dsp][engine][denormal]")
{
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    for (const auto& id : ids)
    {
        INFO("Engine: " << id);
        auto engine = registry.createEngine(id);
        REQUIRE(engine != nullptr);

        engine->prepare(sampleRate, blockSize);
        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);
        bool noDenormals = true;

        // Block 0: note-on
        {
            buffer.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
            engine->renderBlock(buffer, midi, blockSize);
        }

        // Blocks 1-9: sustain
        for (int block = 1; block < 10 && noDenormals; ++block)
        {
            buffer.clear();
            juce::MidiBuffer emptyMidi;
            engine->renderBlock(buffer, emptyMidi, blockSize);
        }

        // Block 10: note-off
        {
            buffer.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 0);
            engine->renderBlock(buffer, midi, blockSize);
        }

        // Blocks 11-49: decay tail — most likely place for denormals
        for (int block = 11; block < 50 && noDenormals; ++block)
        {
            buffer.clear();
            juce::MidiBuffer emptyMidi;
            engine->renderBlock(buffer, emptyMidi, blockSize);

            for (int ch = 0; ch < buffer.getNumChannels() && noDenormals; ++ch)
            {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i)
                    if (isDenormal(data[i]))
                    {
                        noDenormals = false;
                        break;
                    }
            }
        }

        CHECK(noDenormals);
    }
}

