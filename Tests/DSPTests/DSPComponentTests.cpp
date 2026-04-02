/*
    XOlokun DSP Component Tests
    ============================
    Mathematical correctness tests for DSP primitives that go beyond crash
    and stability checks. Tests verify:
      - WavetableOscillator interpolation quality and phase coherency
      - WavetableOscillator morph continuity
      - StandardADSR envelope shape correctness
      - StandardLFO waveform output range and frequency accuracy
      - ParameterSmoother convergence correctness

    These complement the stability tests in DSPStabilityTests.cpp which
    verify that components don't crash or produce NaN/Inf. Here we verify
    that the math is correct.

    No test framework required — assert-based with descriptive console output.
    No JUCE dependency — pure C++ only.

    See: https://github.com/BertCalm/XO_OX-XOmnibus/issues/457
*/

#include "DSPComponentTests.h"

#include "DSP/WavetableOscillator.h"
#include "DSP/StandardADSR.h"
#include "DSP/StandardLFO.h"
#include "DSP/ParameterSmoother.h"
#include "DSP/FastMath.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace xoceanus;

namespace dsp_component_tests {

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_passed = 0;
static int g_failed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        ++g_passed;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        ++g_failed;
    }
}

//==============================================================================
// WavetableOscillator tests
//==============================================================================

static void testWavetableOscillator()
{
    std::cout << "\n--- WavetableOscillator Tests ---\n";
    constexpr float sampleRate = 44100.0f;

    // After generateBasicTables, processSample produces non-zero output
    {
        WavetableOscillator wt;
        wt.generateBasicTables(sampleRate);
        wt.setFrequency(440.0f, sampleRate);
        wt.setMorphPosition(0.0f); // Frame 0: sawtooth

        float sum = 0.0f;
        for (int i = 0; i < 512; ++i)
            sum += std::abs(wt.processSample());
        reportTest("generateBasicTables produces non-zero output", sum > 0.1f);
    }

    // Sine frame (frame 3) should have amplitude close to 1.0
    {
        WavetableOscillator wt;
        wt.generateBasicTables(sampleRate);
        wt.setFrequency(440.0f, sampleRate);
        wt.setMorphPosition(1.0f); // Frame 3: sine

        // Warm up
        for (int i = 0; i < 1000; ++i)
            wt.processSample();

        // Measure peak amplitude over one full period
        float maxAmp = 0.0f;
        int samplesPerPeriod = static_cast<int>(sampleRate / 440.0f) + 10;
        for (int i = 0; i < samplesPerPeriod; ++i)
            maxAmp = std::max(maxAmp, std::abs(wt.processSample()));

        reportTest("Sine frame peak amplitude > 0.7", maxAmp > 0.7f);
        reportTest("Sine frame peak amplitude < 1.5 (not clipping)", maxAmp < 1.5f);
    }

    // Morph continuity: output should not jump discontinuously when morph changes
    {
        WavetableOscillator wt;
        wt.generateBasicTables(sampleRate);
        wt.setFrequency(220.0f, sampleRate);

        // Warm up at position 0.0
        wt.setMorphPosition(0.0f);
        for (int i = 0; i < 500; ++i)
            wt.processSample();

        float beforeMorph = wt.processSample();

        // Change morph position by a small amount
        wt.setMorphPosition(0.01f);
        float afterMorph = wt.processSample();

        // Output should not jump by more than 0.5 for a small morph change
        bool continuous = std::abs(afterMorph - beforeMorph) < 0.5f;
        reportTest("Small morph change produces continuous output (<0.5 jump)", continuous);
    }

    // Phase coherency: same frequency should produce same RMS regardless of morph
    {
        WavetableOscillator wtSaw, wtSine;
        wtSaw.generateBasicTables(sampleRate);
        wtSine.generateBasicTables(sampleRate);

        wtSaw.setFrequency(440.0f, sampleRate);
        wtSine.setFrequency(440.0f, sampleRate);
        wtSaw.setMorphPosition(0.0f);  // Sawtooth
        wtSine.setMorphPosition(1.0f); // Sine

        // Both should produce signals with reasonable RMS (not silence)
        float rmsSaw = 0.0f, rmsSine = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            float s = wtSaw.processSample();
            float si = wtSine.processSample();
            rmsSaw += s * s;
            rmsSine += si * si;
        }
        rmsSaw = std::sqrt(rmsSaw / 1000.0f);
        rmsSine = std::sqrt(rmsSine / 1000.0f);

        reportTest("Sawtooth frame has audible RMS (>0.1)", rmsSaw > 0.1f);
        reportTest("Sine frame has audible RMS (>0.1)", rmsSine > 0.1f);
    }

    // Frequency accuracy: output period should match expected sample count
    {
        WavetableOscillator wt;
        wt.generateBasicTables(sampleRate);
        wt.setMorphPosition(1.0f); // Use sine for cleaner zero-crossing detection
        wt.setFrequency(100.0f, sampleRate);

        // Wait for stable output
        for (int i = 0; i < 2000; ++i)
            wt.processSample();

        // Count zero crossings over 10,000 samples to estimate frequency
        // At 100 Hz and 44100 sr, period = 441 samples, ~22.7 periods in 10k samples
        int crossings = 0;
        float prev = wt.processSample();
        for (int i = 1; i < 10000; ++i)
        {
            float curr = wt.processSample();
            if (prev < 0.0f && curr >= 0.0f)
                ++crossings;
            prev = curr;
        }
        // Should have ~22-23 positive-slope zero crossings
        bool freqAccurate = (crossings >= 18 && crossings <= 28);
        reportTest("Wavetable 100Hz frequency within 25% of target (zero crossings)", freqAccurate);
    }

    // loadWavetable: custom single-frame table is output correctly
    {
        // Create a single-frame sine wave table at kFrameSize
        constexpr int fs = WavetableOscillator::kFrameSize;
        std::vector<float> sineTable(fs);
        for (int i = 0; i < fs; ++i)
            sineTable[i] = std::sin(2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(fs));

        WavetableOscillator wt;
        wt.loadWavetable(sineTable.data(), 1, fs);
        wt.setFrequency(440.0f, sampleRate);
        wt.setMorphPosition(0.0f);

        float maxAmp = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            // Warm up
            wt.processSample();
        }
        for (int i = 0; i < static_cast<int>(sampleRate / 440.0f) + 10; ++i)
            maxAmp = std::max(maxAmp, std::abs(wt.processSample()));

        reportTest("Custom loaded sine table produces output (>0.5 peak)", maxAmp > 0.5f);
    }
}

//==============================================================================
// StandardADSR tests
//==============================================================================

static void testStandardADSR()
{
    std::cout << "\n--- StandardADSR Tests ---\n";
    constexpr float sampleRate = 44100.0f;

    // Envelope starts at 0 before noteOn
    {
        StandardADSR env;
        env.prepare(sampleRate);
        env.setADSR(0.01f, 0.1f, 0.7f, 0.2f);

        float firstSample = env.process();
        reportTest("ADSR starts at 0 before noteOn", firstSample == 0.0f);
    }

    // After noteOn, envelope rises during attack
    {
        StandardADSR env;
        env.prepare(sampleRate);
        env.setADSR(0.1f, 0.1f, 0.7f, 0.2f); // 100ms attack
        env.noteOn();

        // After 50ms (half of attack), output should be between 0 and 1
        int halfAttack = static_cast<int>(sampleRate * 0.05f);
        float midAttack = 0.0f;
        for (int i = 0; i < halfAttack; ++i)
            midAttack = env.process();

        reportTest("ADSR rising during attack (>0.0 at half-attack)", midAttack > 0.0f);
        reportTest("ADSR not yet at peak during attack (<1.1)", midAttack < 1.1f);
    }

    // After attack completes, envelope reaches peak near 1.0
    {
        StandardADSR env;
        env.prepare(sampleRate);
        env.setADSR(0.01f, 0.5f, 0.8f, 0.2f); // 10ms attack, long decay to stay near peak
        env.noteOn();

        int attackSamples = static_cast<int>(sampleRate * 0.015f); // 1.5x attack time
        float peakVal = 0.0f;
        for (int i = 0; i < attackSamples; ++i)
            peakVal = std::max(peakVal, env.process());

        reportTest("ADSR reaches peak ≥ 0.9 after attack", peakVal >= 0.9f);
    }

    // Envelope settles at sustain level
    {
        StandardADSR env;
        env.prepare(sampleRate);
        env.setADSR(0.001f, 0.001f, 0.6f, 0.5f); // Very short A+D
        env.noteOn();

        // Run past A+D, into sustain
        int passSamples = static_cast<int>(sampleRate * 0.05f);
        float sustainVal = 0.0f;
        for (int i = 0; i < passSamples; ++i)
            sustainVal = env.process();

        reportTest("ADSR sustain level within 10% of target (0.6)", std::abs(sustainVal - 0.6f) < 0.1f);
    }

    // After noteOff, envelope decreases
    {
        StandardADSR env;
        env.prepare(sampleRate);
        env.setADSR(0.001f, 0.001f, 0.8f, 0.05f); // 50ms release
        env.noteOn();

        // Run to sustain
        for (int i = 0; i < static_cast<int>(sampleRate * 0.05f); ++i)
            env.process();

        float sustainLevel = env.process();
        env.noteOff();

        // After half of release time, should be lower than sustain
        int halfRelease = static_cast<int>(sampleRate * 0.025f);
        float relVal = sustainLevel;
        for (int i = 0; i < halfRelease; ++i)
            relVal = env.process();

        reportTest("ADSR decreases after noteOff", relVal < sustainLevel);
    }
}

//==============================================================================
// StandardLFO tests
//==============================================================================

static void testStandardLFO()
{
    std::cout << "\n--- StandardLFO Tests ---\n";
    constexpr float sampleRate = 44100.0f;

    // LFO output is in [-1, 1] for bipolar waveforms
    {
        StandardLFO lfo;
        lfo.setRate(2.0f, sampleRate); // 2 Hz
        lfo.setShape(StandardLFO::Shape::Sine);

        float minVal = 2.0f, maxVal = -2.0f;
        for (int i = 0; i < static_cast<int>(sampleRate); ++i) // 1 second
        {
            float v = lfo.process();
            minVal = std::min(minVal, v);
            maxVal = std::max(maxVal, v);
        }
        reportTest("LFO sine output stays below +1.1", maxVal < 1.1f);
        reportTest("LFO sine output stays above -1.1", minVal > -1.1f);
        reportTest("LFO sine reaches positive peak (>0.5)", maxVal > 0.5f);
        reportTest("LFO sine reaches negative peak (<-0.5)", minVal < -0.5f);
    }

    // LFO frequency accuracy: at 1 Hz, ~1 cycle per second
    {
        StandardLFO lfo;
        lfo.setRate(1.0f, sampleRate); // 1 Hz
        lfo.setShape(StandardLFO::Shape::Sine);

        // Count zero crossings (positive slope) over 3 seconds
        int crossings = 0;
        float prev = lfo.process();
        for (int i = 1; i < static_cast<int>(sampleRate * 3.0f); ++i)
        {
            float curr = lfo.process();
            if (prev < 0.0f && curr >= 0.0f)
                ++crossings;
            prev = curr;
        }
        // Should be ~3 positive-slope crossings in 3 seconds at 1 Hz
        reportTest("LFO 1Hz has ~3 cycles in 3s (2-4 crossings)", crossings >= 2 && crossings <= 4);
    }

    // LFO rate change takes effect
    {
        StandardLFO lfo;
        lfo.setShape(StandardLFO::Shape::Sine);

        // Count cycles at 1 Hz over 1 second
        lfo.setRate(1.0f, sampleRate);
        int crossingsSlow = 0;
        float prev = lfo.process();
        for (int i = 1; i < static_cast<int>(sampleRate); ++i)
        {
            float curr = lfo.process();
            if (prev < 0.0f && curr >= 0.0f)
                ++crossingsSlow;
            prev = curr;
        }

        // Count cycles at 5 Hz over 1 second
        lfo.setRate(5.0f, sampleRate);
        int crossingsFast = 0;
        prev = lfo.process();
        for (int i = 1; i < static_cast<int>(sampleRate); ++i)
        {
            float curr = lfo.process();
            if (prev < 0.0f && curr >= 0.0f)
                ++crossingsFast;
            prev = curr;
        }

        reportTest("LFO fast rate produces more cycles than slow rate", crossingsFast > crossingsSlow);
    }
}

//==============================================================================
// ParameterSmoother tests
//==============================================================================

static void testParameterSmoother()
{
    std::cout << "\n--- ParameterSmoother Tests ---\n";
    constexpr float sampleRate = 44100.0f;

    // Smoother converges to target value
    {
        ParameterSmoother smoother;
        smoother.prepare(sampleRate, 0.01f); // 10ms smoothing time

        smoother.set(1.0f);

        // Run for 50ms — well beyond 10ms smoothing time
        float finalVal = 0.0f;
        for (int i = 0; i < static_cast<int>(sampleRate * 0.05f); ++i)
            finalVal = smoother.process();

        reportTest("ParameterSmoother converges to target (>0.95 of 1.0)", finalVal > 0.95f);
    }

    // Smoother starts at initial value (0.0) and doesn't jump
    {
        ParameterSmoother smoother;
        smoother.prepare(sampleRate, 0.1f); // 100ms — slow
        smoother.set(1.0f);

        float firstSample = smoother.process();
        reportTest("ParameterSmoother first sample < 0.1 (no instantaneous jump)", firstSample < 0.1f);
    }

    // Smoother output is monotonically increasing toward target
    {
        ParameterSmoother smoother;
        smoother.prepare(sampleRate, 0.05f);
        smoother.set(1.0f);

        float prev = smoother.process();
        bool monotonic = true;
        for (int i = 0; i < static_cast<int>(sampleRate * 0.1f); ++i)
        {
            float curr = smoother.process();
            if (curr < prev - 0.001f) // Allow tiny floating-point jitter
                monotonic = false;
            prev = curr;
        }
        reportTest("ParameterSmoother output is monotonically increasing", monotonic);
    }
}

//==============================================================================
// Entry point
//==============================================================================

int runAll()
{
    g_passed = 0;
    g_failed = 0;

    std::cout << "========================================\n";
    std::cout << "  DSP Component Tests (Correctness)\n";
    std::cout << "========================================\n";

    testWavetableOscillator();
    testStandardADSR();
    testStandardLFO();
    testParameterSmoother();

    std::cout << "\n  DSP Component Tests: " << g_passed << " passed, "
              << g_failed << " failed\n";

    return g_failed;
}

} // namespace dsp_component_tests
