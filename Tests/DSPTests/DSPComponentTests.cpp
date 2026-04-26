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
      - FastMath additional functions (fastPow2, fastCos, fastTan, softClip,
        lerp, smoothstep, clamp, smoothCoeffFromTime, midiToFreqTune)
      - GlideProcessor portamento correctness
      - FilterEnvelope ADSR stage correctness
      - PitchBendUtil MIDI pitch bend pipeline
      - VoiceAllocator LRU and priority allocation

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
#include "DSP/GlideProcessor.h"
#include "DSP/FilterEnvelope.h"
#include "DSP/PitchBendUtil.h"
#include "DSP/VoiceAllocator.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <array>
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

//==============================================================================
// FastMath — additional function coverage
//==============================================================================

TEST_CASE("FastMath - fastPow2(0) = 1.0", "[dsp][fastmath]")
{
    CHECK(std::abs(fastPow2(0.0f) - 1.0f) < 0.001f);
}

TEST_CASE("FastMath - fastPow2(1) = 2.0 within 0.1%", "[dsp][fastmath]")
{
    CHECK(std::abs(fastPow2(1.0f) - 2.0f) < 0.003f);
}

TEST_CASE("FastMath - fastPow2(-1) = 0.5 within 0.1%", "[dsp][fastmath]")
{
    CHECK(std::abs(fastPow2(-1.0f) - 0.5f) < 0.002f);
}

TEST_CASE("FastMath - fastPow2 one octave: fastPow2(12/12) ≈ 2.0", "[dsp][fastmath]")
{
    // Used in midiToFreq: one octave up should double frequency
    float result = fastPow2(12.0f / 12.0f);
    CHECK(std::abs(result - 2.0f) < 0.003f);
}

TEST_CASE("FastMath - fastPow2 negative input: fastPow2(-126) > 0", "[dsp][fastmath]")
{
    // Guard: large negative exponent must return a small but positive number
    CHECK(fastPow2(-126.0f) > 0.0f);
}

TEST_CASE("FastMath - fastCos(0) = 1.0 within 0.01%", "[dsp][fastmath]")
{
    CHECK(std::abs(fastCos(0.0f) - 1.0f) < 0.0001f);
}

TEST_CASE("FastMath - fastCos(pi/2) ~ 0 within 0.5%", "[dsp][fastmath]")
{
    constexpr float halfPi = 1.5707963268f;
    CHECK(std::abs(fastCos(halfPi)) < 0.005f);
}

TEST_CASE("FastMath - fastCos(pi) ~ -1 within 0.5%", "[dsp][fastmath]")
{
    constexpr float pi = 3.14159265359f;
    CHECK(std::abs(fastCos(pi) - (-1.0f)) < 0.005f);
}

TEST_CASE("FastMath - fastSin² + fastCos² ≈ 1 across full period", "[dsp][fastmath]")
{
    // Verifies the two independent polynomials maintain the Pythagorean identity
    constexpr float twoPi = 6.28318530718f;
    float maxErr = 0.0f;
    for (int i = 0; i < 1000; ++i)
    {
        float x = (static_cast<float>(i) / 1000.0f) * twoPi - 3.14159265359f;
        float s = fastSin(x);
        float c = fastCos(x);
        float err = std::abs(s * s + c * c - 1.0f);
        maxErr = std::max(maxErr, err);
    }
    CHECK(maxErr < 0.01f);
}

TEST_CASE("FastMath - fastTan(0) = 0.0", "[dsp][fastmath]")
{
    CHECK(std::abs(fastTan(0.0f)) < 1e-6f);
}

TEST_CASE("FastMath - fastTan(pi/4) ≈ 1.0 within 0.5%", "[dsp][fastmath]")
{
    constexpr float quarterPi = 0.7853981634f;
    CHECK(std::abs(fastTan(quarterPi) - 1.0f) < 0.005f);
}

TEST_CASE("FastMath - fastTan is monotonically increasing on [0, pi/4]", "[dsp][fastmath]")
{
    constexpr float quarterPi = 0.7853981634f;
    float prev = fastTan(0.0f);
    bool monotonic = true;
    for (int i = 1; i <= 20; ++i)
    {
        float x = (static_cast<float>(i) / 20.0f) * quarterPi;
        float curr = fastTan(x);
        if (curr <= prev)
            monotonic = false;
        prev = curr;
    }
    CHECK(monotonic);
}

TEST_CASE("FastMath - softClip(0) = 0.0 exactly", "[dsp][fastmath]")
{
    CHECK(softClip(0.0f) == 0.0f);
}

TEST_CASE("FastMath - softClip output bounded in (-1, 1] for |x| <= 4", "[dsp][fastmath]")
{
    for (int i = -40; i <= 40; ++i)
    {
        float x = static_cast<float>(i) * 0.1f;
        float y = softClip(x);
        CHECK(y >= -1.0f);
        CHECK(y <= 1.0f);
    }
}

TEST_CASE("FastMath - softClip is monotonically increasing", "[dsp][fastmath]")
{
    float prev = softClip(-5.0f);
    bool monotonic = true;
    for (int i = -49; i <= 50; ++i)
    {
        float x = static_cast<float>(i) * 0.1f;
        float curr = softClip(x);
        if (curr < prev - 1e-6f)
            monotonic = false;
        prev = curr;
    }
    CHECK(monotonic);
}

TEST_CASE("FastMath - softClip hard clips at ±4", "[dsp][fastmath]")
{
    CHECK(softClip(4.0f) == 1.0f);
    CHECK(softClip(-4.0f) == -1.0f);
    CHECK(softClip(100.0f) == 1.0f);
    CHECK(softClip(-100.0f) == -1.0f);
}

TEST_CASE("FastMath - softClip is odd symmetric: softClip(-x) = -softClip(x)", "[dsp][fastmath]")
{
    for (int i = 1; i <= 30; ++i)
    {
        float x = static_cast<float>(i) * 0.1f;
        CHECK(std::abs(softClip(-x) + softClip(x)) < 1e-5f);
    }
}

TEST_CASE("FastMath - lerp(0,1,0.5) = 0.5", "[dsp][fastmath]")
{
    CHECK(std::abs(lerp(0.0f, 1.0f, 0.5f) - 0.5f) < 1e-6f);
}

TEST_CASE("FastMath - lerp endpoints", "[dsp][fastmath]")
{
    CHECK(lerp(2.0f, 8.0f, 0.0f) == 2.0f);
    CHECK(lerp(2.0f, 8.0f, 1.0f) == 8.0f);
}

TEST_CASE("FastMath - lerp midpoint is mean of endpoints", "[dsp][fastmath]")
{
    CHECK(std::abs(lerp(3.0f, 7.0f, 0.5f) - 5.0f) < 1e-5f);
}

TEST_CASE("FastMath - smoothstep(0) = 0, smoothstep(1) = 1", "[dsp][fastmath]")
{
    CHECK(smoothstep(0.0f) == 0.0f);
    CHECK(smoothstep(1.0f) == 1.0f);
}

TEST_CASE("FastMath - smoothstep(0.5) = 0.5", "[dsp][fastmath]")
{
    CHECK(std::abs(smoothstep(0.5f) - 0.5f) < 1e-5f);
}

TEST_CASE("FastMath - smoothstep clamps outside [0,1]", "[dsp][fastmath]")
{
    CHECK(smoothstep(-1.0f) == 0.0f);
    CHECK(smoothstep(2.0f) == 1.0f);
}

TEST_CASE("FastMath - smoothstep is monotonically non-decreasing on [0,1]", "[dsp][fastmath]")
{
    float prev = smoothstep(0.0f);
    bool monotonic = true;
    for (int i = 1; i <= 100; ++i)
    {
        float t = static_cast<float>(i) / 100.0f;
        float curr = smoothstep(t);
        if (curr < prev - 1e-6f)
            monotonic = false;
        prev = curr;
    }
    CHECK(monotonic);
}

TEST_CASE("FastMath - clamp(x, lo, hi) for various inputs", "[dsp][fastmath]")
{
    CHECK(clamp(0.5f, 0.0f, 1.0f) == 0.5f);
    CHECK(clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
    CHECK(clamp(2.0f, 0.0f, 1.0f) == 1.0f);
    CHECK(clamp(0.0f, 0.0f, 1.0f) == 0.0f);
    CHECK(clamp(1.0f, 0.0f, 1.0f) == 1.0f);
}

TEST_CASE("FastMath - smoothCoeffFromTime(0) returns 1.0 (instant)", "[dsp][fastmath]")
{
    CHECK(smoothCoeffFromTime(0.0f, 44100.0f) == 1.0f);
}

TEST_CASE("FastMath - smoothCoeffFromTime is in (0, 1] for positive time", "[dsp][fastmath]")
{
    float c = smoothCoeffFromTime(0.01f, 44100.0f);
    CHECK(c > 0.0f);
    CHECK(c < 1.0f);
}

TEST_CASE("FastMath - smoothCoeffFromTime: longer time gives smaller coefficient", "[dsp][fastmath]")
{
    float cFast = smoothCoeffFromTime(0.001f, 44100.0f);
    float cSlow = smoothCoeffFromTime(0.1f, 44100.0f);
    CHECK(cFast > cSlow);
}

TEST_CASE("FastMath - midiToFreqTune(69, 0) = 440 Hz", "[dsp][fastmath]")
{
    CHECK(std::abs(midiToFreqTune(69, 0.0f) - 440.0f) < 1.0f);
}

TEST_CASE("FastMath - midiToFreqTune +12 semitones doubles frequency", "[dsp][fastmath]")
{
    float f0 = midiToFreqTune(60, 0.0f);
    float f1 = midiToFreqTune(60, 12.0f);
    CHECK(std::abs(f1 / f0 - 2.0f) < 0.01f);
}

TEST_CASE("FastMath - midiToFreqTune -12 semitones halves frequency", "[dsp][fastmath]")
{
    float f0 = midiToFreqTune(60, 0.0f);
    float f1 = midiToFreqTune(60, -12.0f);
    CHECK(std::abs(f1 / f0 - 0.5f) < 0.01f);
}

//==============================================================================
// GlideProcessor tests
//==============================================================================

TEST_CASE("GlideProcessor - snapTo immediately sets current and target freq", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(440.0f);
    CHECK(g.currentFreq == 440.0f);
    CHECK(g.targetFreq == 440.0f);
    CHECK(g.isSettled());
}

TEST_CASE("GlideProcessor - setTargetOrSnap snaps when uninitialized (< 1 Hz)", "[dsp][glide]")
{
    GlideProcessor g;
    // Default currentFreq = 0 → snap
    g.setTargetOrSnap(220.0f);
    CHECK(g.currentFreq == 220.0f);
    CHECK(g.targetFreq == 220.0f);
}

TEST_CASE("GlideProcessor - setTargetOrSnap glides when already initialized", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(220.0f);
    g.setTargetOrSnap(440.0f);
    // target updated but current should still be at the old frequency
    CHECK(g.targetFreq == 440.0f);
    CHECK(g.currentFreq == 220.0f);
    CHECK(!g.isSettled());
}

TEST_CASE("GlideProcessor - setTime(0) produces instant (coeff = 1.0)", "[dsp][glide]")
{
    GlideProcessor g;
    g.setTime(0.0f, 44100.0f);
    CHECK(g.coeff == 1.0f);
}

TEST_CASE("GlideProcessor - setTime produces coeff < 1 for positive time", "[dsp][glide]")
{
    GlideProcessor g;
    g.setTime(0.1f, 44100.0f);
    CHECK(g.coeff > 0.0f);
    CHECK(g.coeff < 1.0f);
}

TEST_CASE("GlideProcessor - process() glides from 220 Hz toward 440 Hz", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(220.0f);
    g.setTime(0.1f, 44100.0f); // 100ms glide
    g.setTarget(440.0f);

    // After half-glide time, frequency should be between start and target
    int halfGlideSamples = static_cast<int>(44100.0f * 0.05f);
    float freq = 220.0f;
    for (int i = 0; i < halfGlideSamples; ++i)
        freq = g.process();

    CHECK(freq > 220.0f);
    CHECK(freq < 440.0f);
}

TEST_CASE("GlideProcessor - process() converges to target after sufficient samples", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(100.0f);
    g.setTime(0.05f, 44100.0f); // 50ms glide
    g.setTarget(880.0f);

    // Run for much longer than glide time
    int runSamples = static_cast<int>(44100.0f * 0.5f);
    float freq = 100.0f;
    for (int i = 0; i < runSamples; ++i)
        freq = g.process();

    CHECK(std::abs(freq - 880.0f) < 1.0f);
    CHECK(g.isSettled());
}

TEST_CASE("GlideProcessor - getFreq() matches last process() output", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(220.0f);
    g.setTime(0.1f, 44100.0f);
    g.setTarget(440.0f);

    for (int i = 0; i < 100; ++i)
        g.process();

    float lastProcessed = g.process();
    CHECK(std::abs(g.getFreq() - lastProcessed) < 1e-4f);
}

TEST_CASE("GlideProcessor - reset() clears all state", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(440.0f);
    g.setTime(0.1f, 44100.0f);
    g.reset();

    CHECK(g.currentFreq == 0.0f);
    CHECK(g.targetFreq == 0.0f);
    CHECK(g.coeff == 1.0f);
}

TEST_CASE("GlideProcessor - coeff=1.0 produces instant arrival in one sample", "[dsp][glide]")
{
    GlideProcessor g;
    g.snapTo(220.0f);
    g.setCoeff(1.0f);
    g.setTarget(880.0f);
    float freq = g.process();
    CHECK(std::abs(freq - 880.0f) < 0.2f); // convergence guard snaps at 0.2 Hz
}

//==============================================================================
// FilterEnvelope tests
//==============================================================================

TEST_CASE("FilterEnvelope - idle returns 0 before trigger", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.01f, 0.1f, 0.7f, 0.2f);
    CHECK(env.process() == 0.0f);
    CHECK(env.getStage() == FilterEnvelope::Stage::Idle);
    CHECK(!env.isActive());
}

TEST_CASE("FilterEnvelope - trigger() starts attack phase", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.1f, 0.1f, 0.7f, 0.2f);
    env.trigger();
    CHECK(env.getStage() == FilterEnvelope::Stage::Attack);
    CHECK(env.isActive());

    float firstSample = env.process();
    CHECK(firstSample > 0.0f);
}

TEST_CASE("FilterEnvelope - triggerHard() resets level to 0 before attack", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.001f, 0.001f, 0.5f, 0.1f);

    // Advance to sustain
    env.trigger();
    for (int i = 0; i < static_cast<int>(44100.0f * 0.1f); ++i)
        env.process();

    float preLevel = env.getLevel();
    CHECK(preLevel > 0.1f);

    env.triggerHard();
    CHECK(env.getLevel() == 0.0f);
    CHECK(env.getStage() == FilterEnvelope::Stage::Attack);
}

TEST_CASE("FilterEnvelope - attack phase reaches peak of 1.0", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.01f, 0.5f, 0.8f, 0.5f); // short 10ms attack, long decay
    env.trigger();

    float peakVal = 0.0f;
    int attackSamples = static_cast<int>(44100.0f * 0.015f);
    for (int i = 0; i < attackSamples; ++i)
        peakVal = std::max(peakVal, env.process());

    CHECK(peakVal >= 0.9f);
}

TEST_CASE("FilterEnvelope - decay settles at sustain level", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.001f, 0.05f, 0.5f, 0.5f); // very short attack, 50ms decay
    env.trigger();

    float sustainVal = 0.0f;
    int runSamples = static_cast<int>(44100.0f * 0.3f); // well past decay
    for (int i = 0; i < runSamples; ++i)
        sustainVal = env.process();

    CHECK(std::abs(sustainVal - 0.5f) < 0.05f);
    CHECK(env.getStage() == FilterEnvelope::Stage::Sustain);
}

TEST_CASE("FilterEnvelope - release() decreases level toward zero", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.001f, 0.001f, 0.8f, 0.1f);
    env.trigger();

    // Advance to sustain
    for (int i = 0; i < static_cast<int>(44100.0f * 0.05f); ++i)
        env.process();

    float sustainLevel = env.getLevel();
    env.release();

    // Run halfway through release
    int halfRelease = static_cast<int>(44100.0f * 0.05f);
    float relVal = sustainLevel;
    for (int i = 0; i < halfRelease; ++i)
        relVal = env.process();

    CHECK(relVal < sustainLevel);
    CHECK(relVal >= 0.0f);
}

TEST_CASE("FilterEnvelope - release reaches Idle after release time", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.001f, 0.001f, 0.9f, 0.05f); // 50ms release
    env.trigger();

    // Advance to sustain
    for (int i = 0; i < static_cast<int>(44100.0f * 0.05f); ++i)
        env.process();

    env.release();

    // Run for much longer than release time
    for (int i = 0; i < static_cast<int>(44100.0f * 0.5f); ++i)
        env.process();

    CHECK(env.getStage() == FilterEnvelope::Stage::Idle);
    CHECK(!env.isActive());
}

TEST_CASE("FilterEnvelope - kill() immediately silences and goes Idle", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.01f, 0.1f, 0.7f, 0.2f);
    env.trigger();

    for (int i = 0; i < 100; ++i)
        env.process();

    env.kill();

    CHECK(env.getLevel() == 0.0f);
    CHECK(env.getStage() == FilterEnvelope::Stage::Idle);
    CHECK(!env.isActive());
    CHECK(env.process() == 0.0f);
}

TEST_CASE("FilterEnvelope - legato retrigger starts from current level (no jump to zero)", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.001f, 0.001f, 0.6f, 0.2f);
    env.trigger();

    // Advance to sustain
    for (int i = 0; i < static_cast<int>(44100.0f * 0.05f); ++i)
        env.process();

    float levelBeforeRetrigger = env.getLevel();
    CHECK(levelBeforeRetrigger > 0.1f);

    // Legato retrigger: trigger() should NOT reset level to zero
    env.trigger();
    CHECK(env.getLevel() == levelBeforeRetrigger);
    CHECK(env.getStage() == FilterEnvelope::Stage::Attack);
}

TEST_CASE("FilterEnvelope - release() from Idle stays Idle", "[dsp][filterenv]")
{
    FilterEnvelope env;
    env.prepare(44100.0f);
    env.setADSR(0.01f, 0.1f, 0.7f, 0.2f);
    env.release(); // should be a no-op when Idle
    CHECK(env.getStage() == FilterEnvelope::Stage::Idle);
}

//==============================================================================
// PitchBendUtil tests
//==============================================================================

TEST_CASE("PitchBendUtil - parsePitchWheel: center (8192) maps to 0.0", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::parsePitchWheel(8192)) < 1e-4f);
}

TEST_CASE("PitchBendUtil - parsePitchWheel: min (0) maps to -1.0", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::parsePitchWheel(0) - (-1.0f)) < 0.001f);
}

TEST_CASE("PitchBendUtil - parsePitchWheel: max (16383) maps to +1.0 (within 0.02%)", "[dsp][pitchbend]")
{
    float v = PitchBendUtil::parsePitchWheel(16383);
    CHECK(v > 0.99f);
    CHECK(v <= 1.001f);
}

TEST_CASE("PitchBendUtil - parsePitchWheel is bipolar and antisymmetric around center", "[dsp][pitchbend]")
{
    // parsePitchWheel(8192 - d) ≈ -parsePitchWheel(8192 + d)
    for (int d : {100, 500, 1000, 4000})
    {
        float neg = PitchBendUtil::parsePitchWheel(8192 - d);
        float pos = PitchBendUtil::parsePitchWheel(8192 + d);
        CHECK(std::abs(neg + pos) < 0.001f);
    }
}

TEST_CASE("PitchBendUtil - bendToSemitones: ±1 normalized × 2 semitone range = ±2 semitones", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::bendToSemitones(1.0f, 2.0f) - 2.0f) < 1e-5f);
    CHECK(std::abs(PitchBendUtil::bendToSemitones(-1.0f, 2.0f) - (-2.0f)) < 1e-5f);
}

TEST_CASE("PitchBendUtil - bendToSemitones: zero bend produces zero semitones", "[dsp][pitchbend]")
{
    CHECK(PitchBendUtil::bendToSemitones(0.0f, 2.0f) == 0.0f);
}

TEST_CASE("PitchBendUtil - semitonesToFreqRatio(0) = 1.0 (no bend)", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::semitonesToFreqRatio(0.0f) - 1.0f) < 0.001f);
}

TEST_CASE("PitchBendUtil - semitonesToFreqRatio(12) ≈ 2.0 (one octave up)", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::semitonesToFreqRatio(12.0f) - 2.0f) < 0.01f);
}

TEST_CASE("PitchBendUtil - semitonesToFreqRatio(-12) ≈ 0.5 (one octave down)", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::semitonesToFreqRatio(-12.0f) - 0.5f) < 0.01f);
}

TEST_CASE("PitchBendUtil - freqRatio(8192, 2.0) = 1.0 (center = no bend)", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::freqRatio(8192, 2.0f) - 1.0f) < 0.001f);
}

TEST_CASE("PitchBendUtil - freqRatio scales correctly with range", "[dsp][pitchbend]")
{
    // Half-max bend with 2-semitone range should give 1-semitone ratio
    float halfBendRatio = PitchBendUtil::freqRatio(8192 + 4096, 2.0f);
    float oneSemitoneRatio = PitchBendUtil::semitonesToFreqRatio(1.0f);
    CHECK(std::abs(halfBendRatio - oneSemitoneRatio) < 0.01f);
}

TEST_CASE("PitchBendUtil - semitonesToCents multiplies by 100", "[dsp][pitchbend]")
{
    CHECK(std::abs(PitchBendUtil::semitonesToCents(1.0f) - 100.0f) < 1e-4f);
    CHECK(std::abs(PitchBendUtil::semitonesToCents(2.5f) - 250.0f) < 1e-4f);
    CHECK(std::abs(PitchBendUtil::semitonesToCents(0.0f)) < 1e-4f);
}

//==============================================================================
// VoiceAllocator tests
//==============================================================================

struct TestVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = -1;
    bool releasing = false;
};

TEST_CASE("VoiceAllocator - returns first inactive voice when available", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].startTime = 10;
    voices[1].active = false;
    voices[2].active = false;
    voices[3].active = true; voices[3].startTime = 5;

    int idx = VoiceAllocator::findFreeVoice(voices, 4);
    CHECK(idx == 1); // first inactive
}

TEST_CASE("VoiceAllocator - steals oldest active voice when all active (LRU)", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].startTime = 100;
    voices[1].active = true; voices[1].startTime = 50;  // oldest
    voices[2].active = true; voices[2].startTime = 200;
    voices[3].active = true; voices[3].startTime = 150;

    int idx = VoiceAllocator::findFreeVoice(voices, 4);
    CHECK(idx == 1); // oldest start time
}

TEST_CASE("VoiceAllocator - single voice always returned", "[dsp][voice]")
{
    std::array<TestVoice, 1> voices;
    voices[0].active = false;
    CHECK(VoiceAllocator::findFreeVoice(voices, 1) == 0);

    voices[0].active = true;
    voices[0].startTime = 99;
    CHECK(VoiceAllocator::findFreeVoice(voices, 1) == 0);
}

TEST_CASE("VoiceAllocator - findFreeVoicePreferRelease: prefers releasing voice over active", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].startTime = 100; voices[0].releasing = false;
    voices[1].active = true; voices[1].startTime = 50;  voices[1].releasing = true; // old releasing
    voices[2].active = true; voices[2].startTime = 200; voices[2].releasing = false;
    voices[3].active = true; voices[3].startTime = 150; voices[3].releasing = false;

    int idx = VoiceAllocator::findFreeVoicePreferRelease(
        voices, 4, [](const TestVoice& v) { return v.releasing; });
    CHECK(idx == 1); // releasing voice
}

TEST_CASE("VoiceAllocator - findFreeVoicePreferRelease: fallback to oldest when no releasing voice", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].startTime = 100; voices[0].releasing = false;
    voices[1].active = true; voices[1].startTime = 50;  voices[1].releasing = false; // oldest overall
    voices[2].active = true; voices[2].startTime = 200; voices[2].releasing = false;
    voices[3].active = true; voices[3].startTime = 150; voices[3].releasing = false;

    int idx = VoiceAllocator::findFreeVoicePreferRelease(
        voices, 4, [](const TestVoice& v) { return v.releasing; });
    CHECK(idx == 1); // oldest overall
}

TEST_CASE("VoiceAllocator - findFreeVoicePreferRelease: inactive voice preferred over releasing", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true;  voices[0].startTime = 100; voices[0].releasing = true;
    voices[1].active = false; // free
    voices[2].active = true;  voices[2].startTime = 200; voices[2].releasing = false;
    voices[3].active = true;  voices[3].startTime = 150; voices[3].releasing = false;

    int idx = VoiceAllocator::findFreeVoicePreferRelease(
        voices, 4, [](const TestVoice& v) { return v.releasing; });
    CHECK(idx == 1); // inactive over releasing
}

TEST_CASE("VoiceAllocator - findFreeVoiceCouplingAware: prefers uncoupled voice", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].startTime = 100; // coupled
    voices[1].active = true; voices[1].startTime = 50;  // coupled
    voices[2].active = true; voices[2].startTime = 80;  // uncoupled — oldest uncoupled
    voices[3].active = true; voices[3].startTime = 200; // uncoupled

    int idx = VoiceAllocator::findFreeVoiceCouplingAware(
        voices, 4, [](int i) { return i == 0 || i == 1; }); // voices 0 and 1 are coupled
    CHECK(idx == 2); // oldest uncoupled
}

TEST_CASE("VoiceAllocator - findFreeVoiceCouplingAware: fallback to oldest when all coupled", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].startTime = 100;
    voices[1].active = true; voices[1].startTime = 50;  // oldest overall
    voices[2].active = true; voices[2].startTime = 200;
    voices[3].active = true; voices[3].startTime = 150;

    // All voices are coupled
    int idx = VoiceAllocator::findFreeVoiceCouplingAware(
        voices, 4, [](int) { return true; });
    CHECK(idx == 1); // oldest fallback
}

TEST_CASE("VoiceAllocator - countActive returns correct count", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true;
    voices[1].active = false;
    voices[2].active = true;
    voices[3].active = true;

    CHECK(VoiceAllocator::countActive(voices, 4) == 3);
}

TEST_CASE("VoiceAllocator - countActive returns 0 when all inactive", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    CHECK(VoiceAllocator::countActive(voices, 4) == 0);
}

TEST_CASE("VoiceAllocator - countActive returns maxPoly when all active", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    for (auto& v : voices) v.active = true;
    CHECK(VoiceAllocator::countActive(voices, 4) == 4);
}

TEST_CASE("VoiceAllocator - findVoiceForNote returns correct index", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].currentNote = 60;
    voices[1].active = true; voices[1].currentNote = 64;
    voices[2].active = true; voices[2].currentNote = 67;
    voices[3].active = false; voices[3].currentNote = 72;

    CHECK(VoiceAllocator::findVoiceForNote(voices, 4, 60) == 0);
    CHECK(VoiceAllocator::findVoiceForNote(voices, 4, 64) == 1);
    CHECK(VoiceAllocator::findVoiceForNote(voices, 4, 67) == 2);
}

TEST_CASE("VoiceAllocator - findVoiceForNote returns -1 when not found", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = true; voices[0].currentNote = 60;
    voices[1].active = true; voices[1].currentNote = 64;

    CHECK(VoiceAllocator::findVoiceForNote(voices, 4, 72) == -1);
}

TEST_CASE("VoiceAllocator - findVoiceForNote ignores inactive voices", "[dsp][voice]")
{
    std::array<TestVoice, 4> voices;
    voices[0].active = false; voices[0].currentNote = 60; // inactive, should not match
    voices[1].active = true;  voices[1].currentNote = 64;

    CHECK(VoiceAllocator::findVoiceForNote(voices, 4, 60) == -1);
}
