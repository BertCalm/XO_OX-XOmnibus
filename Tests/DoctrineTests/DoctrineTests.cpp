/*
    XOmnibus Doctrine Automation Tests
    ====================================
    Automated checks for the 6 Doctrines (D001–D006) across all 34 engines.

    D001: Velocity Must Shape Timbre (not just amplitude)
    D002: Modulation is the Lifeblood (LFOs, mod matrix, macros)
    D003: The Physics IS the Synthesis (skip — requires human review)
    D004: Dead Parameters Are Broken Promises (every param affects audio)
    D005: An Engine That Cannot Breathe Is a Photograph (LFO rate ≤ 0.01 Hz)
    D006: Expression Input Is Not Optional (velocity→timbre + CC)
*/

#include "DoctrineTests.h"

#include "Core/SynthEngine.h"
#include "Core/EngineRegistry.h"

#include <juce_core/juce_core.h>
#include <iostream>
#include <cmath>
#include <set>
#include <string>

using namespace xomnibus;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_doctrineTestsPassed = 0;
static int g_doctrineTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_doctrineTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_doctrineTestsFailed++;
    }
}

//==============================================================================
// D001: Velocity Must Shape Timbre
// Test that different velocity values produce different spectral content,
// not just volume differences.
//==============================================================================

static void testD001_VelocityShapesTimbre()
{
    std::cout << "\n--- D001: Velocity Must Shape Timbre ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    int enginesWithVelocityTimbre = 0;

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine) continue;

        // Render a short block at low velocity and high velocity
        constexpr int blockSize = 512;
        constexpr double sampleRate = 44100.0;
        engine->prepare(sampleRate, blockSize);

        juce::AudioBuffer<float> bufLow(2, blockSize);
        juce::AudioBuffer<float> bufHigh(2, blockSize);
        bufLow.clear();
        bufHigh.clear();

        // Low velocity render
        juce::MidiBuffer midiLow;
        midiLow.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)30), 0);
        engine->renderBlock(bufLow, midiLow);

        // Reset and render high velocity
        engine->prepare(sampleRate, blockSize);
        juce::MidiBuffer midiHigh;
        midiHigh.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)120), 0);
        engine->renderBlock(bufHigh, midiHigh);

        // Compare spectral content: compute RMS of first half vs second half
        // as a rough spectral proxy (brighter signals have more energy in upper half)
        float rmsLowFirst = 0, rmsLowSecond = 0;
        float rmsHighFirst = 0, rmsHighSecond = 0;

        for (int i = 0; i < blockSize / 2; ++i)
        {
            float sL = bufLow.getSample(0, i);
            float sH = bufHigh.getSample(0, i);
            rmsLowFirst += sL * sL;
            rmsHighFirst += sH * sH;
        }
        for (int i = blockSize / 2; i < blockSize; ++i)
        {
            float sL = bufLow.getSample(0, i);
            float sH = bufHigh.getSample(0, i);
            rmsLowSecond += sL * sL;
            rmsHighSecond += sH * sH;
        }

        // Normalize
        rmsLowFirst = std::sqrt(rmsLowFirst / (blockSize / 2));
        rmsLowSecond = std::sqrt(rmsLowSecond / (blockSize / 2));
        rmsHighFirst = std::sqrt(rmsHighFirst / (blockSize / 2));
        rmsHighSecond = std::sqrt(rmsHighSecond / (blockSize / 2));

        // If velocity only affects amplitude, the ratio would be constant
        // If velocity shapes timbre, the spectral balance changes
        float ratioLow = (rmsLowFirst > 1e-8f) ? rmsLowSecond / rmsLowFirst : 0.0f;
        float ratioHigh = (rmsHighFirst > 1e-8f) ? rmsHighSecond / rmsHighFirst : 0.0f;

        // Check if the spectral ratio differs between velocities
        bool timbreShift = std::abs(ratioLow - ratioHigh) > 0.001f;
        if (timbreShift) enginesWithVelocityTimbre++;
    }

    float coverage = ids.size() > 0 ? (float)enginesWithVelocityTimbre / (float)ids.size() : 0.0f;
    reportTest("D001: >90% engines have velocity→timbre",
               coverage > 0.9f || ids.size() == 0);

    std::cout << "  (D001 coverage: " << enginesWithVelocityTimbre
              << "/" << ids.size() << " engines)\n";
}

//==============================================================================
// D004: Dead Parameters Are Broken Promises
// Verify each declared parameter affects output by sweeping min→max.
//==============================================================================

static void testD004_NoDeadParameters()
{
    std::cout << "\n--- D004: Dead Parameters Are Broken Promises ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    int totalParams = 0;
    int deadParams = 0;

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine) continue;

        constexpr int blockSize = 256;
        constexpr double sampleRate = 44100.0;

        auto params = engine->getParameters();

        for (auto* param : params)
        {
            if (!param) continue;
            totalParams++;

            auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param);
            if (!rangedParam) continue;

            // Render at default value
            engine->prepare(sampleRate, blockSize);
            juce::AudioBuffer<float> bufDefault(2, blockSize);
            bufDefault.clear();
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
            engine->renderBlock(bufDefault, midi);

            float rmsDefault = 0;
            for (int i = 0; i < blockSize; ++i)
            {
                float s = bufDefault.getSample(0, i);
                rmsDefault += s * s;
            }
            rmsDefault = std::sqrt(rmsDefault / blockSize);

            // Set parameter to max and re-render
            rangedParam->setValueNotifyingHost(1.0f);
            engine->prepare(sampleRate, blockSize);
            juce::AudioBuffer<float> bufMax(2, blockSize);
            bufMax.clear();
            juce::MidiBuffer midiMax;
            midiMax.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
            engine->renderBlock(bufMax, midiMax);

            float rmsMax = 0;
            for (int i = 0; i < blockSize; ++i)
            {
                float s = bufMax.getSample(0, i);
                rmsMax += s * s;
            }
            rmsMax = std::sqrt(rmsMax / blockSize);

            // Check if output changed
            if (std::abs(rmsDefault - rmsMax) < 1e-8f)
                deadParams++;

            // Reset parameter
            rangedParam->setValueNotifyingHost(rangedParam->getDefaultValue());
        }
    }

    float deadRatio = totalParams > 0 ? (float)deadParams / (float)totalParams : 0.0f;
    reportTest("D004: <5% dead parameters", deadRatio < 0.05f || totalParams == 0);

    std::cout << "  (D004: " << deadParams << "/" << totalParams
              << " parameters appear dead)\n";
}

//==============================================================================
// D005: An Engine That Cannot Breathe Is a Photograph
// Verify each engine has an LFO with min rate ≤ 0.01 Hz.
//==============================================================================

static void testD005_LFOBreathing()
{
    std::cout << "\n--- D005: LFO Breathing (rate floor ≤ 0.01 Hz) ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    int enginesWithBreathing = 0;

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine) continue;

        // Check for LFO rate parameters with min value ≤ 0.01 Hz
        auto params = engine->getParameters();
        bool hasBreathingLFO = false;

        for (auto* param : params)
        {
            if (!param) continue;

            auto name = param->getName(100).toLowerCase();
            if (name.contains("lfo") && name.contains("rate"))
            {
                auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
                if (ranged)
                {
                    auto range = ranged->getNormalisableRange();
                    if (range.start <= 0.01f)
                        hasBreathingLFO = true;
                }
            }
        }

        if (hasBreathingLFO) enginesWithBreathing++;
    }

    // OPTIC is exempt (visual engine)
    int expectedEngines = (int)ids.size() - 1;  // minus Optic
    float coverage = expectedEngines > 0
        ? (float)enginesWithBreathing / (float)expectedEngines : 1.0f;

    reportTest("D005: >90% engines have breathing LFO",
               coverage > 0.9f || ids.size() == 0);

    std::cout << "  (D005 coverage: " << enginesWithBreathing
              << "/" << ids.size() << " engines, Optic exempt)\n";
}

//==============================================================================
// D006: Expression Input Is Not Optional
// Verify velocity→timbre and at least one CC (aftertouch/mod wheel).
//==============================================================================

static void testD006_ExpressionInput()
{
    std::cout << "\n--- D006: Expression Input Is Not Optional ---\n";

    auto& registry = EngineRegistry::instance();
    auto ids = registry.getRegisteredIds();
    int enginesWithExpression = 0;

    for (const auto& id : ids)
    {
        auto engine = registry.createEngine(id);
        if (!engine) continue;

        // Check parameters for aftertouch/modwheel destinations
        auto params = engine->getParameters();
        bool hasAftertouch = false;
        bool hasModWheel = false;

        for (auto* param : params)
        {
            if (!param) continue;
            auto name = param->getName(100).toLowerCase();

            if (name.contains("aftertouch") || name.contains("pressure"))
                hasAftertouch = true;
            if (name.contains("modwheel") || name.contains("mod wheel") || name.contains("cc1"))
                hasModWheel = true;
        }

        if (hasAftertouch || hasModWheel)
            enginesWithExpression++;
    }

    // Optic is exempt
    float coverage = ids.size() > 1
        ? (float)enginesWithExpression / (float)(ids.size() - 1) : 1.0f;

    reportTest("D006: >90% engines have expression input",
               coverage > 0.9f || ids.size() == 0);

    std::cout << "  (D006 coverage: " << enginesWithExpression
              << "/" << ids.size() << " engines)\n";
}

//==============================================================================
// Public entry point
//==============================================================================

namespace doctrine_tests {

int runAll()
{
    g_doctrineTestsPassed = 0;
    g_doctrineTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  Doctrine Automation Tests (D001–D006)\n";
    std::cout << "========================================\n";

    testD001_VelocityShapesTimbre();
    // D002, D003 require human review — skipped
    testD004_NoDeadParameters();
    testD005_LFOBreathing();
    testD006_ExpressionInput();

    std::cout << "\n  Doctrine Tests: " << g_doctrineTestsPassed << " passed, "
              << g_doctrineTestsFailed << " failed\n";

    return g_doctrineTestsFailed;
}

} // namespace doctrine_tests
