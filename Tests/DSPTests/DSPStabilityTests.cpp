/*
    XOmnibus DSP Stability Tests
    =============================
    Tests for FastMath, CytomicSVF, PolyBLEP, and per-engine rendering stability.
    No test framework — assert-based with descriptive console output.
*/

#include "DSPStabilityTests.h"

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
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <functional>
#include <algorithm>

using namespace xomnibus;

//==============================================================================
// Engine registration for the test binary.
// The main plugin registers engines in XOmnibusProcessor.cpp via static init.
// We must register them here for the standalone test executable.
//==============================================================================

static bool reg_OddfeliX   = EngineRegistry::instance().registerEngine("OddfeliX",   []() -> std::unique_ptr<SynthEngine> { return std::make_unique<SnapEngine>(); });
static bool reg_OddOscar   = EngineRegistry::instance().registerEngine("OddOscar",   []() -> std::unique_ptr<SynthEngine> { return std::make_unique<MorphEngine>(); });
static bool reg_Overdub     = EngineRegistry::instance().registerEngine("Overdub",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<DubEngine>(); });
static bool reg_Odyssey     = EngineRegistry::instance().registerEngine("Odyssey",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<DriftEngine>(); });
static bool reg_Oblong      = EngineRegistry::instance().registerEngine("Oblong",      []() -> std::unique_ptr<SynthEngine> { return std::make_unique<BobEngine>(); });
static bool reg_Obese       = EngineRegistry::instance().registerEngine("Obese",       []() -> std::unique_ptr<SynthEngine> { return std::make_unique<FatEngine>(); });
static bool reg_Onset       = EngineRegistry::instance().registerEngine("Onset",       []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OnsetEngine>(); });
static bool reg_Overworld   = EngineRegistry::instance().registerEngine("Overworld",   []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OverworldEngine>(); });
static bool reg_Opal        = EngineRegistry::instance().registerEngine("Opal",        []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OpalEngine>(); });
static bool reg_Bite        = EngineRegistry::instance().registerEngine("Bite",        []() -> std::unique_ptr<SynthEngine> { return std::make_unique<BiteEngine>(); });
static bool reg_Organon     = EngineRegistry::instance().registerEngine("Organon",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OrganonEngine>(); });
static bool reg_Ocelot      = EngineRegistry::instance().registerEngine("Ocelot",      []() -> std::unique_ptr<SynthEngine> { return std::make_unique<xocelot::OcelotEngine>(); });
static bool reg_Ouroboros   = EngineRegistry::instance().registerEngine("Ouroboros",   []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OuroborosEngine>(); });
static bool reg_Obsidian    = EngineRegistry::instance().registerEngine("Obsidian",    []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObsidianEngine>(); });
static bool reg_Origami     = EngineRegistry::instance().registerEngine("Origami",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OrigamiEngine>(); });
static bool reg_Oracle      = EngineRegistry::instance().registerEngine("Oracle",      []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OracleEngine>(); });
static bool reg_Obscura     = EngineRegistry::instance().registerEngine("Obscura",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObscuraEngine>(); });
static bool reg_Oceanic     = EngineRegistry::instance().registerEngine("Oceanic",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OceanicEngine>(); });
static bool reg_Optic       = EngineRegistry::instance().registerEngine("Optic",       []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OpticEngine>(); });
static bool reg_Oblique     = EngineRegistry::instance().registerEngine("Oblique",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObliqueEngine>(); });
static bool reg_Orbital     = EngineRegistry::instance().registerEngine("Orbital",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OrbitalEngine>(); });
static bool reg_Osprey      = EngineRegistry::instance().registerEngine("Osprey",      []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OspreyEngine>(); });
static bool reg_Osteria     = EngineRegistry::instance().registerEngine("Osteria",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OsteriaEngine>(); });
static bool reg_Owlfish     = EngineRegistry::instance().registerEngine("Owlfish",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<xowlfish::OwlfishEngine>(); });
static bool reg_Ohm         = EngineRegistry::instance().registerEngine("Ohm",         []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OhmEngine>(); });
static bool reg_Orphica     = EngineRegistry::instance().registerEngine("Orphica",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OrphicaEngine>(); });
static bool reg_Obbligato   = EngineRegistry::instance().registerEngine("Obbligato",   []() -> std::unique_ptr<SynthEngine> { return std::make_unique<ObbligatoEngine>(); });
static bool reg_Ottoni      = EngineRegistry::instance().registerEngine("Ottoni",      []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OttoniEngine>(); });
static bool reg_Ole         = EngineRegistry::instance().registerEngine("Ole",         []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OleEngine>(); });
static bool reg_XOverlap    = EngineRegistry::instance().registerEngine("XOverlap",    []() -> std::unique_ptr<SynthEngine> { return std::make_unique<XOverlapEngine>(); });
static bool reg_XOutwit     = EngineRegistry::instance().registerEngine("XOutwit",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<XOutwitEngine>(); });
static bool reg_Ombre       = EngineRegistry::instance().registerEngine("Ombre",       []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OmbreEngine>(); });
static bool reg_Orca        = EngineRegistry::instance().registerEngine("Orca",        []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OrcaEngine>(); });
static bool reg_Octopus     = EngineRegistry::instance().registerEngine("Octopus",     []() -> std::unique_ptr<SynthEngine> { return std::make_unique<OctopusEngine>(); });

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_dspTestsPassed = 0;
static int g_dspTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_dspTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_dspTestsFailed++;
    }
}

static bool isFinite(float v)
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

static void testFastMath()
{
    std::cout << "\n--- FastMath Tests ---\n";

    // flushDenormal: denormal -> 0
    {
        // Construct a denormal float
        float denorm = std::numeric_limits<float>::denorm_min();
        float result = flushDenormal(denorm);
        reportTest("flushDenormal returns 0 for denormal", result == 0.0f);
    }

    // flushDenormal: normal preserved
    {
        float normal = 1.0f;
        float result = flushDenormal(normal);
        reportTest("flushDenormal preserves normal value (1.0)", result == 1.0f);
    }

    // flushDenormal: zero preserved
    {
        float result = flushDenormal(0.0f);
        reportTest("flushDenormal preserves zero", result == 0.0f);
    }

    // flushDenormal: negative normal preserved
    {
        float result = flushDenormal(-42.5f);
        reportTest("flushDenormal preserves negative normal", result == -42.5f);
    }

    // fastExp accuracy within 5% of std::exp for [-10, 10]
    {
        bool withinTolerance = true;
        float worstError = 0.0f;
        for (float x = -10.0f; x <= 10.0f; x += 0.1f)
        {
            float fast = fastExp(x);
            float ref = std::exp(x);
            if (ref > 1e-6f)
            {
                float relErr = std::abs(fast - ref) / ref;
                worstError = std::max(worstError, relErr);
                if (relErr > 0.05f)
                    withinTolerance = false;
            }
        }
        reportTest("fastExp within 5% of std::exp for [-10,10]", withinTolerance);
    }

    // fastTanh accuracy within 1% of std::tanh for [-3, 3]
    {
        bool withinTolerance = true;
        for (float x = -3.0f; x <= 3.0f; x += 0.05f)
        {
            float fast = fastTanh(x);
            float ref = std::tanh(x);
            float err = std::abs(fast - ref);
            // Use absolute error since tanh range is [-1,1]
            if (err > 0.01f)
                withinTolerance = false;
        }
        reportTest("fastTanh within 1% of std::tanh for [-3,3]", withinTolerance);
    }

    // fastSin accuracy within 1% of std::sin for full period
    {
        bool withinTolerance = true;
        constexpr float twoPi = 6.28318530717958647692f;
        for (float x = -twoPi; x <= twoPi; x += 0.01f)
        {
            float fast = fastSin(x);
            float ref = std::sin(x);
            float err = std::abs(fast - ref);
            if (err > 0.01f)
                withinTolerance = false;
        }
        reportTest("fastSin within 1% of std::sin for full period", withinTolerance);
    }

    // midiToFreq: note 69 = 440 Hz
    {
        float freq = midiToFreq(69);
        bool close = std::abs(freq - 440.0f) < 0.01f;
        reportTest("midiToFreq(69) = 440 Hz", close);
    }

    // dbToGain: 0 dB = 1.0
    {
        float gain = dbToGain(0.0f);
        bool close = std::abs(gain - 1.0f) < 0.001f;
        reportTest("dbToGain(0) = 1.0", close);
    }

    // dbToGain: -6 dB ~ 0.5
    {
        float gain = dbToGain(-6.0f);
        bool close = std::abs(gain - 0.5012f) < 0.01f; // exact: 10^(-6/20) ~ 0.5012
        reportTest("dbToGain(-6) ~ 0.5", close);
    }

    // gainToDb: gain 1.0 = 0 dB
    {
        float db = gainToDb(1.0f);
        bool close = std::abs(db) < 0.001f;
        reportTest("gainToDb(1.0) = 0 dB", close);
    }
}

//==============================================================================
// CytomicSVF tests
//==============================================================================

static void testCytomicSVF()
{
    std::cout << "\n--- CytomicSVF Tests ---\n";
    constexpr float sampleRate = 44100.0f;

    // After reset, processSample(0) returns 0
    {
        CytomicSVF filter;
        filter.setMode(CytomicSVF::Mode::LowPass);
        filter.setCoefficients(1000.0f, 0.5f, sampleRate);
        filter.reset();
        float out = filter.processSample(0.0f);
        reportTest("After reset, processSample(0) = 0", out == 0.0f);
    }

    // LowPass at 1000Hz passes 100Hz signal (>0.9 amplitude)
    {
        CytomicSVF filter;
        filter.setMode(CytomicSVF::Mode::LowPass);
        filter.setCoefficients(1000.0f, 0.0f, sampleRate);
        filter.reset();

        // Run 100Hz sine through the filter and measure steady-state amplitude
        constexpr float freq = 100.0f;
        constexpr float twoPi = 6.28318530717958647692f;
        float maxOut = 0.0f;

        // Warm up for 1000 samples, then measure
        for (int i = 0; i < 5000; ++i)
        {
            float in = std::sin(twoPi * freq * static_cast<float>(i) / sampleRate);
            float out = filter.processSample(in);
            if (i > 2000)
                maxOut = std::max(maxOut, std::abs(out));
        }
        reportTest("LowPass@1000Hz passes 100Hz (>0.9)", maxOut > 0.9f);
    }

    // LowPass at 1000Hz attenuates 10kHz signal (<0.1 amplitude)
    {
        CytomicSVF filter;
        filter.setMode(CytomicSVF::Mode::LowPass);
        filter.setCoefficients(1000.0f, 0.0f, sampleRate);
        filter.reset();

        constexpr float freq = 10000.0f;
        constexpr float twoPi = 6.28318530717958647692f;
        float maxOut = 0.0f;

        for (int i = 0; i < 5000; ++i)
        {
            float in = std::sin(twoPi * freq * static_cast<float>(i) / sampleRate);
            float out = filter.processSample(in);
            if (i > 2000)
                maxOut = std::max(maxOut, std::abs(out));
        }
        reportTest("LowPass@1000Hz attenuates 10kHz (<0.1)", maxOut < 0.1f);
    }

    // BandPass at 1000Hz passes 1000Hz signal
    {
        CytomicSVF filter;
        filter.setMode(CytomicSVF::Mode::BandPass);
        filter.setCoefficients(1000.0f, 0.5f, sampleRate);
        filter.reset();

        constexpr float freq = 1000.0f;
        constexpr float twoPi = 6.28318530717958647692f;
        float maxOut = 0.0f;

        for (int i = 0; i < 5000; ++i)
        {
            float in = std::sin(twoPi * freq * static_cast<float>(i) / sampleRate);
            float out = filter.processSample(in);
            if (i > 2000)
                maxOut = std::max(maxOut, std::abs(out));
        }
        reportTest("BandPass@1000Hz passes 1000Hz (>0.1)", maxOut > 0.1f);
    }

    // No NaN/Inf after 10000 samples of noise input
    {
        CytomicSVF filter;
        filter.setMode(CytomicSVF::Mode::LowPass);
        filter.setCoefficients(5000.0f, 0.8f, sampleRate);
        filter.reset();

        juce::Random rng(42);
        bool stable = true;
        for (int i = 0; i < 10000; ++i)
        {
            float in = rng.nextFloat() * 2.0f - 1.0f;
            float out = filter.processSample(in);
            if (!isFinite(out))
            {
                stable = false;
                break;
            }
        }
        reportTest("No NaN/Inf after 10000 noise samples", stable);
    }

    // Stability: self-oscillation (resonance=1.0) doesn't blow up after 100000 samples
    {
        CytomicSVF filter;
        filter.setMode(CytomicSVF::Mode::LowPass);
        filter.setCoefficients(1000.0f, 1.0f, sampleRate);
        filter.reset();

        bool stable = true;
        for (int i = 0; i < 100000; ++i)
        {
            // Feed tiny impulse then silence
            float in = (i == 0) ? 1.0f : 0.0f;
            float out = filter.processSample(in);
            if (!isFinite(out) || std::abs(out) > 10.0f)
            {
                stable = false;
                break;
            }
        }
        reportTest("Self-oscillation (res=1.0) stable for 100000 samples", stable);
    }
}

//==============================================================================
// PolyBLEP tests
//==============================================================================

static void testPolyBLEP()
{
    std::cout << "\n--- PolyBLEP Tests ---\n";
    constexpr float sampleRate = 44100.0f;

    // All waveforms in [-1.1, 1.1] range
    {
        PolyBLEP::Waveform waveforms[] = {
            PolyBLEP::Waveform::Sine,
            PolyBLEP::Waveform::Saw,
            PolyBLEP::Waveform::Square,
            PolyBLEP::Waveform::Triangle,
            PolyBLEP::Waveform::Pulse
        };
        const char* names[] = { "Sine", "Saw", "Square", "Triangle", "Pulse" };

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
            std::string testName = std::string(names[w]) + " waveform in [-1.1, 1.1]";
            reportTest(testName.c_str(), inRange);
        }
    }

    // Saw wave at 440Hz: reasonable RMS (0.3-0.9)
    {
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
        reportTest("Saw@440Hz RMS in [0.3, 0.9]", rms >= 0.3f && rms <= 0.9f);
    }

    // Square wave at 440Hz: reasonable RMS
    {
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
        reportTest("Square@440Hz RMS in [0.3, 1.1]", rms >= 0.3f && rms <= 1.1f);
    }

    // Sine wave at 440Hz is within 5% of std::sin at same phase
    {
        PolyBLEP osc;
        osc.setWaveform(PolyBLEP::Waveform::Sine);
        osc.setFrequency(440.0f, sampleRate);
        osc.reset();

        constexpr float twoPi = 6.28318530717958647692f;
        bool withinTolerance = true;
        float phaseInc = 440.0f / sampleRate;
        float phase = 0.0f;

        for (int i = 0; i < 1000; ++i)
        {
            float oscOut = osc.processSample();
            float ref = std::sin(twoPi * phase);
            float err = std::abs(oscOut - ref);
            if (err > 0.05f)
                withinTolerance = false;
            phase += phaseInc;
            while (phase >= 1.0f) phase -= 1.0f;
        }
        reportTest("Sine@440Hz within 5% of std::sin", withinTolerance);
    }

    // No NaN/Inf in any waveform after 100000 samples
    {
        PolyBLEP::Waveform waveforms[] = {
            PolyBLEP::Waveform::Sine,
            PolyBLEP::Waveform::Saw,
            PolyBLEP::Waveform::Square,
            PolyBLEP::Waveform::Triangle,
            PolyBLEP::Waveform::Pulse
        };

        bool allStable = true;
        for (int w = 0; w < 5; ++w)
        {
            PolyBLEP osc;
            osc.setWaveform(waveforms[w]);
            osc.setFrequency(440.0f, sampleRate);
            osc.reset();

            for (int i = 0; i < 100000; ++i)
            {
                float s = osc.processSample();
                if (!isFinite(s))
                {
                    allStable = false;
                    break;
                }
            }
            if (!allStable) break;
        }
        reportTest("No NaN/Inf in any waveform after 100000 samples", allStable);
    }
}

//==============================================================================
// Engine rendering stability tests
//==============================================================================

// Helper: test a single engine for rendering stability
static void testEngineStability(const char* engineName,
                                std::unique_ptr<SynthEngine> engine)
{
    if (!engine)
    {
        std::string msg = std::string(engineName) + ": engine creation";
        reportTest(msg.c_str(), false);
        return;
    }

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    // prepare() / reset() don't crash
    {
        engine->prepare(sampleRate, blockSize);
        engine->reset();
        std::string msg = std::string(engineName) + ": prepare/reset don't crash";
        reportTest(msg.c_str(), true);
    }

    // renderBlock() with empty MIDI: no NaN/Inf
    {
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        juce::MidiBuffer emptyMidi;

        engine->renderBlock(buffer, emptyMidi, blockSize);

        bool stable = true;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
            {
                if (!isFinite(data[i]))
                {
                    stable = false;
                    break;
                }
            }
            if (!stable) break;
        }
        std::string msg = std::string(engineName) + ": empty MIDI produces no NaN/Inf";
        reportTest(msg.c_str(), stable);
    }

    // renderBlock() with note-on produces non-silent output
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

        // Check that at least some non-silent output was produced.
        // Some engines may need more blocks to ramp up, so also check
        // a second block if the first was silent.
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

        std::string msg = std::string(engineName) + ": note-on produces non-silent output";
        reportTest(msg.c_str(), maxSample > 1e-6f);
    }

    // After reset(), getSampleForCoupling() returns 0
    {
        engine->reset();
        float cL = engine->getSampleForCoupling(0, 0);
        float cR = engine->getSampleForCoupling(1, 0);
        bool zeroed = (cL == 0.0f && cR == 0.0f);
        std::string msg = std::string(engineName) + ": after reset, coupling samples = 0";
        reportTest(msg.c_str(), zeroed);
    }

    // No denormals in output after 10 blocks
    {
        engine->reset();
        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

        bool noDenormals = true;
        for (int block = 0; block < 10; ++block)
        {
            buffer.clear();
            juce::MidiBuffer blockMidi;
            if (block == 0)
                blockMidi = midi;
            engine->renderBlock(buffer, blockMidi, blockSize);

            for (int ch = 0; ch < buffer.getNumChannels() && noDenormals; ++ch)
            {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i)
                {
                    if (isDenormal(data[i]))
                    {
                        noDenormals = false;
                        break;
                    }
                }
            }
        }
        std::string msg = std::string(engineName) + ": no denormals after 10 blocks";
        reportTest(msg.c_str(), noDenormals);
    }
}

static void testEngineRendering()
{
    std::cout << "\n--- Engine Rendering Stability Tests (All Registered Engines) ---\n";

    // Iterate over every engine in the EngineRegistry and test each one.
    // This guarantees coverage grows automatically as new engines are registered.
    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();

    // Sort for deterministic output order
    std::sort(ids.begin(), ids.end());

    std::cout << "  Registered engines: " << ids.size() << "\n";

    // Verify we have all 34 expected engines
    {
        bool has34 = (ids.size() >= 34);
        std::string msg = "Registry contains >= 34 engines (found " + std::to_string(ids.size()) + ")";
        reportTest(msg.c_str(), has34);
    }

    // Test each registered engine for rendering stability
    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        testEngineStability(id.c_str(), std::move(engine));
    }
}

//==============================================================================
// Registry-based creation tests — verify every engine creates successfully
// and that createEngine returns nullptr for unknown IDs.
//==============================================================================

static void testRegistryCreation()
{
    std::cout << "\n--- Engine Registry Creation Tests ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    // Each registered ID must create a non-null engine
    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        std::string msg = id + ": createEngine returns non-null";
        reportTest(msg.c_str(), engine != nullptr);

        if (engine)
        {
            // Verify getEngineId() returns a non-empty string
            juce::String eid = engine->getEngineId();
            std::string msg2 = id + ": getEngineId() is non-empty";
            reportTest(msg2.c_str(), eid.isNotEmpty());
        }
    }

    // Unknown ID returns nullptr
    {
        auto engine = registry.createEngine("NonExistentEngine_XYZ");
        reportTest("Unknown engine ID returns nullptr", engine == nullptr);
    }
}

//==============================================================================
// Fleet-wide denormal protection test — renders 50 blocks through each engine
// with note-on then note-off to catch denormals in decay tails.
//==============================================================================

static void testFleetDenormalProtection()
{
    std::cout << "\n--- Fleet-Wide Denormal Protection ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    std::sort(ids.begin(), ids.end());

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine)
        {
            std::string msg = id + ": denormal test skipped (null engine)";
            reportTest(msg.c_str(), false);
            continue;
        }

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
                {
                    if (isDenormal(data[i]))
                    {
                        noDenormals = false;
                        break;
                    }
                }
            }
        }

        std::string msg = id + ": no denormals in 50-block note-on/off cycle";
        reportTest(msg.c_str(), noDenormals);
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace dsp_tests {

int runAll()
{
    g_dspTestsPassed = 0;
    g_dspTestsFailed = 0;

    std::cout << "========================================\n";
    std::cout << "  DSP Stability Tests\n";
    std::cout << "========================================\n";

    testFastMath();
    testCytomicSVF();
    testPolyBLEP();
    testRegistryCreation();
    testEngineRendering();
    testFleetDenormalProtection();

    std::cout << "\n  DSP Tests: " << g_dspTestsPassed << " passed, "
              << g_dspTestsFailed << " failed\n";

    return g_dspTestsFailed;
}

} // namespace dsp_tests
