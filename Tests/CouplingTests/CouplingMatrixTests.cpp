/*
    XOlokun Coupling Matrix Tests
    ===============================
    Tests for MegaCouplingMatrix route management and processing.
    No test framework — assert-based with descriptive console output.
*/

#include "CouplingMatrixTests.h"

#include "Core/SynthEngine.h"
#include "Core/MegaCouplingMatrix.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cmath>

using namespace xolokun;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_couplingTestsPassed = 0;
static int g_couplingTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_couplingTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_couplingTestsFailed++;
    }
}

//==============================================================================
// Minimal test engine — implements SynthEngine for coupling matrix testing.
//==============================================================================

class TestEngine : public SynthEngine
{
public:
    TestEngine(const juce::String& id = "TestEngine") : engineId(id) {}

    void prepare(double /*sampleRate*/, int maxBlockSize) override
    {
        outputBuffer.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        couplingInputReceived = false;
        lastCouplingType = CouplingType::AmpToFilter;
        lastCouplingAmount = 0.0f;
        lastCouplingNumSamples = 0;
    }

    void renderBlock(juce::AudioBuffer<float>& /*buffer*/,
                     juce::MidiBuffer& /*midi*/,
                     int /*numSamples*/) override
    {
        // No-op for test engine
    }

    float getSampleForCoupling(int /*channel*/, int sampleIndex) const override
    {
        auto idx = static_cast<size_t>(sampleIndex);
        if (idx < outputBuffer.size())
            return outputBuffer[idx];
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* /*sourceBuffer*/, int numSamples) override
    {
        couplingInputReceived = true;
        lastCouplingType = type;
        lastCouplingAmount = amount;
        lastCouplingNumSamples = numSamples;
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        return {};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& /*apvts*/) override {}

    juce::String getEngineId() const override { return engineId; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFFFFFFF); }
    int getMaxVoices() const override { return 1; }

    // Fill output buffer with a constant value (for coupling reads)
    void fillOutput(float value)
    {
        std::fill(outputBuffer.begin(), outputBuffer.end(), value);
    }

    // Public state for assertions
    bool couplingInputReceived = false;
    CouplingType lastCouplingType = CouplingType::AmpToFilter;
    float lastCouplingAmount = 0.0f;
    int lastCouplingNumSamples = 0;

private:
    juce::String engineId;
    std::vector<float> outputBuffer;
};

//==============================================================================
// Tests
//==============================================================================

static void testPrepare()
{
    std::cout << "\n--- Coupling Matrix Prepare ---\n";

    MegaCouplingMatrix matrix;
    matrix.prepare(512);
    reportTest("prepare() initializes without crash", true);
}

static void testRouteManagement()
{
    std::cout << "\n--- Route Management ---\n";

    // addRoute adds routes visible in getRoutes
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(512);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::AmpToFilter;
        route.amount = 0.5f;
        route.isNormalled = false;
        route.active = true;

        matrix.addRoute(route);
        auto routes = matrix.getRoutes();
        reportTest("addRoute: route appears in getRoutes", routes.size() == 1);
    }

    // Add multiple routes
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(512);

        for (int i = 0; i < 3; ++i)
        {
            MegaCouplingMatrix::CouplingRoute route;
            route.sourceSlot = 0;
            route.destSlot = i + 1;
            route.type = CouplingType::AmpToFilter;
            route.amount = 0.5f;
            route.isNormalled = false;
            route.active = true;
            matrix.addRoute(route);
        }
        auto routes = matrix.getRoutes();
        reportTest("addRoute: multiple routes added correctly", routes.size() == 3);
    }

    // removeUserRoute removes user routes but preserves normalled
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(512);

        // Add a normalled route
        MegaCouplingMatrix::CouplingRoute normalled;
        normalled.sourceSlot = 0;
        normalled.destSlot = 1;
        normalled.type = CouplingType::AmpToFilter;
        normalled.amount = 0.3f;
        normalled.isNormalled = true;
        normalled.active = false; // disabled by user override
        matrix.addRoute(normalled);

        // Add a user route (same source/dest/type)
        MegaCouplingMatrix::CouplingRoute userRoute;
        userRoute.sourceSlot = 0;
        userRoute.destSlot = 1;
        userRoute.type = CouplingType::AmpToFilter;
        userRoute.amount = 0.8f;
        userRoute.isNormalled = false;
        userRoute.active = true;
        matrix.addRoute(userRoute);

        // Remove user route
        matrix.removeUserRoute(0, 1, CouplingType::AmpToFilter);

        auto routes = matrix.getRoutes();
        bool hasNormalled = false;
        bool hasUser = false;
        for (const auto& r : routes)
        {
            if (r.isNormalled) hasNormalled = true;
            if (!r.isNormalled) hasUser = true;
        }
        reportTest("removeUserRoute: user route removed", !hasUser);
        reportTest("removeUserRoute: normalled route preserved", hasNormalled);

        // Check normalled route was re-enabled
        bool normalledActive = false;
        for (const auto& r : routes)
            if (r.isNormalled)
                normalledActive = r.active;
        reportTest("removeUserRoute: normalled route re-enabled", normalledActive);
    }

    // clearRoutes empties all
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(512);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::AmpToFilter;
        route.amount = 0.5f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);
        matrix.addRoute(route);

        matrix.clearRoutes();
        auto routes = matrix.getRoutes();
        reportTest("clearRoutes: all routes removed", routes.empty());
    }
}

static void testProcessBlock()
{
    std::cout << "\n--- Process Block ---\n";
    constexpr int blockSize = 256;

    // processBlock with no routes is a no-op
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine src("Src"), dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        reportTest("processBlock: no routes = no-op (no crash)", true);
        reportTest("processBlock: no routes = no coupling input",
                   !dst.couplingInputReceived);
    }

    // processBlock with an active route calls applyCouplingInput on destination
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine src("Src"), dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.fillOutput(0.5f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::AmpToFilter;
        route.amount = 0.7f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        reportTest("processBlock: active route calls applyCouplingInput",
                   dst.couplingInputReceived);
        reportTest("processBlock: correct coupling type",
                   dst.lastCouplingType == CouplingType::AmpToFilter);
        reportTest("processBlock: correct coupling amount",
                   std::abs(dst.lastCouplingAmount - 0.7f) < 0.001f);
        reportTest("processBlock: correct numSamples",
                   dst.lastCouplingNumSamples == blockSize);
    }

    // Route amount < 0.001 is treated as inactive
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine src("Src"), dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.fillOutput(0.5f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::AmpToFilter;
        route.amount = 0.0005f; // below threshold
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        reportTest("Route amount < 0.001 treated as inactive",
                   !dst.couplingInputReceived);
    }

    // Out-of-bounds slot indices don't crash
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine eng("Eng");
        eng.prepare(44100.0, blockSize);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &eng, nullptr, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = -1;  // out of bounds
        route.destSlot = 99;    // out of bounds
        route.type = CouplingType::AmpToFilter;
        route.amount = 0.5f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);
        reportTest("Out-of-bounds slot indices don't crash", true);
    }

    // Null engine pointers in slots don't crash
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        // All engine pointers are null
        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { nullptr, nullptr, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::AmpToFilter;
        route.amount = 0.5f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);
        reportTest("Null engine pointers in slots don't crash", true);
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace coupling_tests {

int runAll()
{
    g_couplingTestsPassed = 0;
    g_couplingTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  Coupling Matrix Tests\n";
    std::cout << "========================================\n";

    testPrepare();
    testRouteManagement();
    testProcessBlock();

    std::cout << "\n  Coupling Tests: " << g_couplingTestsPassed << " passed, "
              << g_couplingTestsFailed << " failed\n";

    return g_couplingTestsFailed;
}

} // namespace coupling_tests
