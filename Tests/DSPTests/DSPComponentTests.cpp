/*
    XOceanus DSP Component Tests
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

    No JUCE dependency — pure C++ only.

    See: https://github.com/BertCalm/XO_OX-XOmnibus/issues/457
    Migrated to Catch2 v3: issue #81
*/

#include "DSPComponentTests.h"

#include <catch2/catch_test_macros.hpp>

#include "DSP/WavetableOscillator.h"
#include "DSP/StandardADSR.h"
#include "DSP/StandardLFO.h"
#include "DSP/ParameterSmoother.h"
#include "DSP/FastMath.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace xoceanus;

//==============================================================================
// WavetableOscillator tests
//==============================================================================

TEST_CASE("WavetableOscillator - generateBasicTables produces non-zero output", "[dsp][wavetable]")
{
    constexpr float sampleRate = 44100.0f;
    WavetableOscillator wt;
    wt.generateBasicTables(sampleRate);
    wt.setFrequency(440.0f, sampleRate);
    wt.setMorphPosition(0.0f); // Frame 0: sawtooth

    float sum = 0.0f;
    for (int i = 0; i < 512; ++i)
        sum += std::abs(wt.processSample());
    CHECK(sum > 0.1f);
}

TEST_CASE("WavetableOscillator - sine frame amplitude is close to 1.0", "[dsp][wavetable]")
{
    constexpr float sampleRate = 44100.0f;
    WavetableOscillator wt;
    wt.generateBasicTables(sampleRate);
    wt.setFrequency(440.0f, sampleRate);
    wt.setMorphPosition(1.0f); // Frame 3: sine

    // Warm up
    for (int i = 0; i < 1000; ++i)
        wt.processSample();

    float maxAmp = 0.0f;
    int samplesPerPeriod = static_cast<int>(sampleRate / 440.0f) + 10;
    for (int i = 0; i < samplesPerPeriod; ++i)
        maxAmp = std::max(maxAmp, std::abs(wt.processSample()));

    CHECK(maxAmp > 0.7f);
    CHECK(maxAmp < 1.5f);
}

TEST_CASE("WavetableOscillator - small morph change produces continuous output", "[dsp][wavetable]")
{
    constexpr float sampleRate = 44100.0f;
    WavetableOscillator wt;
    wt.generateBasicTables(sampleRate);
    wt.setFrequency(220.0f, sampleRate);

    wt.setMorphPosition(0.0f);
    for (int i = 0; i < 500; ++i)
        wt.processSample();

    float beforeMorph = wt.processSample();
    wt.setMorphPosition(0.01f);
    float afterMorph = wt.processSample();

    CHECK(std::abs(afterMorph - beforeMorph) < 0.5f);
}

TEST_CASE("WavetableOscillator - sawtooth and sine frames both have audible RMS", "[dsp][wavetable]")
{
    constexpr float sampleRate = 44100.0f;
    WavetableOscillator wtSaw, wtSine;
    wtSaw.generateBasicTables(sampleRate);
    wtSine.generateBasicTables(sampleRate);

    wtSaw.setFrequency(440.0f, sampleRate);
    wtSine.setFrequency(440.0f, sampleRate);
    wtSaw.setMorphPosition(0.0f);
    wtSine.setMorphPosition(1.0f);

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

    CHECK(rmsSaw > 0.1f);
    CHECK(rmsSine > 0.1f);
}

TEST_CASE("WavetableOscillator - 100Hz frequency within 25% of target", "[dsp][wavetable]")
{
    constexpr float sampleRate = 44100.0f;
    WavetableOscillator wt;
    wt.generateBasicTables(sampleRate);
    wt.setMorphPosition(1.0f); // sine for clean zero-crossings
    wt.setFrequency(100.0f, sampleRate);

    for (int i = 0; i < 2000; ++i)
        wt.processSample();

    int crossings = 0;
    float prev = wt.processSample();
    for (int i = 1; i < 10000; ++i)
    {
        float curr = wt.processSample();
        if (prev < 0.0f && curr >= 0.0f)
            ++crossings;
        prev = curr;
    }
    // At 100 Hz / 44100 sr, ~22-23 positive-slope zero crossings in 10k samples
    CHECK(crossings >= 18);
    CHECK(crossings <= 28);
}

TEST_CASE("WavetableOscillator - custom loaded sine table produces output", "[dsp][wavetable]")
{
    constexpr float sampleRate = 44100.0f;
    constexpr int fs = WavetableOscillator::kFrameSize;
    std::vector<float> sineTable(fs);
    for (int i = 0; i < fs; ++i)
        sineTable[i] = std::sin(2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(fs));

    WavetableOscillator wt;
    wt.loadWavetable(sineTable.data(), 1, fs);
    wt.setFrequency(440.0f, sampleRate);
    wt.setMorphPosition(0.0f);

    for (int i = 0; i < 1000; ++i)
        wt.processSample();

    float maxAmp = 0.0f;
    for (int i = 0; i < static_cast<int>(sampleRate / 440.0f) + 10; ++i)
        maxAmp = std::max(maxAmp, std::abs(wt.processSample()));

    CHECK(maxAmp > 0.5f);
}

//==============================================================================
// StandardADSR tests
//==============================================================================

TEST_CASE("StandardADSR - starts at 0 before noteOn", "[dsp][adsr]")
{
    constexpr float sampleRate = 44100.0f;
    StandardADSR env;
    env.prepare(sampleRate);
    env.setADSR(0.01f, 0.1f, 0.7f, 0.2f);

    float firstSample = env.process();
    CHECK(firstSample == 0.0f);
}

TEST_CASE("StandardADSR - rises during attack phase", "[dsp][adsr]")
{
    constexpr float sampleRate = 44100.0f;
    StandardADSR env;
    env.prepare(sampleRate);
    env.setADSR(0.1f, 0.1f, 0.7f, 0.2f); // 100ms attack
    env.noteOn();

    int halfAttack = static_cast<int>(sampleRate * 0.05f);
    float midAttack = 0.0f;
    for (int i = 0; i < halfAttack; ++i)
        midAttack = env.process();

    CHECK(midAttack > 0.0f);
    CHECK(midAttack < 1.1f);
}

TEST_CASE("StandardADSR - reaches peak >= 0.9 after attack", "[dsp][adsr]")
{
    constexpr float sampleRate = 44100.0f;
    StandardADSR env;
    env.prepare(sampleRate);
    env.setADSR(0.01f, 0.5f, 0.8f, 0.2f); // 10ms attack, long decay
    env.noteOn();

    int attackSamples = static_cast<int>(sampleRate * 0.015f);
    float peakVal = 0.0f;
    for (int i = 0; i < attackSamples; ++i)
        peakVal = std::max(peakVal, env.process());

    CHECK(peakVal >= 0.9f);
}

TEST_CASE("StandardADSR - settles at sustain level", "[dsp][adsr]")
{
    constexpr float sampleRate = 44100.0f;
    StandardADSR env;
    env.prepare(sampleRate);
    env.setADSR(0.001f, 0.001f, 0.6f, 0.5f); // Very short A+D
    env.noteOn();

    int passSamples = static_cast<int>(sampleRate * 0.05f);
    float sustainVal = 0.0f;
    for (int i = 0; i < passSamples; ++i)
        sustainVal = env.process();

    CHECK(std::abs(sustainVal - 0.6f) < 0.1f);
}

TEST_CASE("StandardADSR - decreases after noteOff", "[dsp][adsr]")
{
    constexpr float sampleRate = 44100.0f;
    StandardADSR env;
    env.prepare(sampleRate);
    env.setADSR(0.001f, 0.001f, 0.8f, 0.05f); // 50ms release
    env.noteOn();

    for (int i = 0; i < static_cast<int>(sampleRate * 0.05f); ++i)
        env.process();

    float sustainLevel = env.process();
    env.noteOff();

    int halfRelease = static_cast<int>(sampleRate * 0.025f);
    float relVal = sustainLevel;
    for (int i = 0; i < halfRelease; ++i)
        relVal = env.process();

    CHECK(relVal < sustainLevel);
}

//==============================================================================
// StandardLFO tests
//==============================================================================

TEST_CASE("StandardLFO - sine output stays within [-1.1, +1.1]", "[dsp][lfo]")
{
    constexpr float sampleRate = 44100.0f;
    StandardLFO lfo;
    lfo.setRate(2.0f, sampleRate);
    lfo.setShape(StandardLFO::Shape::Sine);

    float minVal = 2.0f, maxVal = -2.0f;
    for (int i = 0; i < static_cast<int>(sampleRate); ++i)
    {
        float v = lfo.process();
        minVal = std::min(minVal, v);
        maxVal = std::max(maxVal, v);
    }
    CHECK(maxVal < 1.1f);
    CHECK(minVal > -1.1f);
    CHECK(maxVal > 0.5f);
    CHECK(minVal < -0.5f);
}

TEST_CASE("StandardLFO - 1Hz sine has ~3 cycles in 3 seconds", "[dsp][lfo]")
{
    constexpr float sampleRate = 44100.0f;
    StandardLFO lfo;
    lfo.setRate(1.0f, sampleRate);
    lfo.setShape(StandardLFO::Shape::Sine);

    int crossings = 0;
    float prev = lfo.process();
    for (int i = 1; i < static_cast<int>(sampleRate * 3.0f); ++i)
    {
        float curr = lfo.process();
        if (prev < 0.0f && curr >= 0.0f)
            ++crossings;
        prev = curr;
    }
    CHECK(crossings >= 2);
    CHECK(crossings <= 4);
}

TEST_CASE("StandardLFO - rate change produces more cycles at faster rate", "[dsp][lfo]")
{
    constexpr float sampleRate = 44100.0f;
    StandardLFO lfo;
    lfo.setShape(StandardLFO::Shape::Sine);

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

    CHECK(crossingsFast > crossingsSlow);
}

//==============================================================================
// ParameterSmoother tests
//==============================================================================

TEST_CASE("ParameterSmoother - converges to target value", "[dsp][smoother]")
{
    constexpr float sampleRate = 44100.0f;
    ParameterSmoother smoother;
    smoother.prepare(sampleRate, 0.01f); // 10ms smoothing
    smoother.set(1.0f);

    float finalVal = 0.0f;
    for (int i = 0; i < static_cast<int>(sampleRate * 0.05f); ++i)
        finalVal = smoother.process();

    CHECK(finalVal > 0.95f);
}

TEST_CASE("ParameterSmoother - no instantaneous jump on first sample", "[dsp][smoother]")
{
    constexpr float sampleRate = 44100.0f;
    ParameterSmoother smoother;
    smoother.prepare(sampleRate, 0.1f); // 100ms — slow
    smoother.set(1.0f);

    float firstSample = smoother.process();
    CHECK(firstSample < 0.1f);
}

TEST_CASE("ParameterSmoother - output is monotonically increasing toward target", "[dsp][smoother]")
{
    constexpr float sampleRate = 44100.0f;
    ParameterSmoother smoother;
    smoother.prepare(sampleRate, 0.05f);
    smoother.set(1.0f);

    float prev = smoother.process();
    bool monotonic = true;
    for (int i = 0; i < static_cast<int>(sampleRate * 0.1f); ++i)
    {
        float curr = smoother.process();
        if (curr < prev - 0.001f)
            monotonic = false;
        prev = curr;
    }
    CHECK(monotonic);
}

// Backward-compat shim — no longer used by run_tests.cpp but kept
// so any hypothetical direct callers don't break at link time.
namespace dsp_component_tests
{
int runAll()
{
    return 0;
}
} // namespace dsp_component_tests
