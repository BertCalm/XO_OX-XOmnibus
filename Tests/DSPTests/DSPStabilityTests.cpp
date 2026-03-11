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
#include "Engines/Snap/SnapEngine.h"
#include "Engines/Morph/MorphEngine.h"
#include "Engines/Dub/DubEngine.h"
#include "Engines/Drift/DriftEngine.h"
#include "Engines/Bob/BobEngine.h"
#include "Engines/Fat/FatEngine.h"
#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Overworld/OverworldEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <functional>

using namespace xomnibus;

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
    std::cout << "\n--- Engine Rendering Stability Tests ---\n";

    // Create and test each engine. All engines handle null APVTS params gracefully
    // by using default values when parameter pointers are null.

    testEngineStability("Snap", std::make_unique<SnapEngine>());
    testEngineStability("Morph", std::make_unique<MorphEngine>());
    testEngineStability("Dub", std::make_unique<DubEngine>());
    testEngineStability("Drift", std::make_unique<DriftEngine>());
    testEngineStability("Bob", std::make_unique<BobEngine>());
    testEngineStability("Fat", std::make_unique<FatEngine>());
    testEngineStability("Onset", std::make_unique<OnsetEngine>());
    testEngineStability("Overworld", std::make_unique<OverworldEngine>());
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
    testEngineRendering();

    std::cout << "\n  DSP Tests: " << g_dspTestsPassed << " passed, "
              << g_dspTestsFailed << " failed\n";

    return g_dspTestsFailed;
}

} // namespace dsp_tests
