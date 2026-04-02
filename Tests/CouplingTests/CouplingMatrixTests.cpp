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
        triangularCouplingReceived = false;
        lastTriangularState = {};
        lastTriangularAmount = 0.0f;
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

    // Override applyTriangularCouplingInput to track state for assertions.
    // Also calls the base-class fallback so we can verify that path works too.
    void applyTriangularCouplingInput(LoveTriangleState state, float amount) override
    {
        triangularCouplingReceived = true;
        lastTriangularState = state;
        lastTriangularAmount = amount;
        // Invoke the default AmpToFilter fallback so applyCouplingInput is exercised.
        SynthEngine::applyTriangularCouplingInput(state, amount);
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

    // TriangularCoupling state for assertions
    bool triangularCouplingReceived = false;
    LoveTriangleState lastTriangularState {};
    float lastTriangularAmount = 0.0f;

private:
    juce::String engineId;
    std::vector<float> outputBuffer;
};

// Test engine that simulates an Oxytocin-like source with real I/P/C state.
class OxytocinLikeTestEngine : public TestEngine
{
public:
    OxytocinLikeTestEngine() : TestEngine("OxytocinLike") {}

    void setLoveState(float I, float P, float C) { loveState = { I, P, C }; }

    LoveTriangleState getLoveTriangleState() const override { return loveState; }

private:
    LoveTriangleState loveState { 0.7f, 0.5f, 0.3f };
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

// Tests for KnotTopology bidirectional coupling (issue #434)
static void testKnotTopology()
{
    std::cout << "\n--- KnotTopology Bidirectional Coupling ---\n";
    constexpr int blockSize = 256;

    // KnotTopology must call applyCouplingInput on BOTH engines (bidirectional).
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine src("Src"), dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.fillOutput(0.5f);
        dst.fillOutput(0.3f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::KnotTopology;
        route.amount = 0.6f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        // processKnotRoute dispatches as AmpToFilter in both directions.
        // Both engines must have received coupling input.
        reportTest("KnotTopology: dest engine receives coupling",
                   dst.couplingInputReceived);
        reportTest("KnotTopology: src engine also receives coupling (bidirectional)",
                   src.couplingInputReceived);
        // processKnotRoute passes CouplingType::AmpToFilter to each engine.
        reportTest("KnotTopology: dest receives AmpToFilter semantics",
                   dst.lastCouplingType == CouplingType::AmpToFilter);
        reportTest("KnotTopology: src receives AmpToFilter semantics",
                   src.lastCouplingType == CouplingType::AmpToFilter);
    }

    // KnotTopology self-route must be a no-op (no crash, no coupling).
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine eng("Self");
        eng.prepare(44100.0, blockSize);
        eng.fillOutput(0.5f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &eng, nullptr, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        // Self-route should be blocked by addRoute's guard.
        MegaCouplingMatrix::CouplingRoute selfRoute;
        selfRoute.sourceSlot = 0;
        selfRoute.destSlot = 0;
        selfRoute.type = CouplingType::KnotTopology;
        selfRoute.amount = 0.5f;
        selfRoute.isNormalled = false;
        selfRoute.active = true;
        matrix.addRoute(selfRoute);

        reportTest("KnotTopology: self-route blocked by addRoute",
                   matrix.getRoutes().empty());
    }

    // KnotTopology scales with linking number encoded in amount.
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine src("Src"), dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.fillOutput(0.5f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        // amount=1.0 → linkingNum=5, scaledAmount=1.0
        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::KnotTopology;
        route.amount = 1.0f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        // scaledAmount = linkingNum(5) / 5 = 1.0
        reportTest("KnotTopology: linking number 5 yields scaledAmount=1.0",
                   std::abs(dst.lastCouplingAmount - 1.0f) < 0.01f);
    }
}

// Tests for TriangularCoupling semantic I/P/C routing (issues #420 and #434)
static void testTriangularCoupling()
{
    std::cout << "\n--- TriangularCoupling Semantic I/P/C Routing ---\n";
    constexpr int blockSize = 256;

    // TriangularCoupling must call applyTriangularCouplingInput, NOT applyCouplingInput.
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        OxytocinLikeTestEngine src;
        TestEngine dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.setLoveState(0.8f, 0.6f, 0.4f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::TriangularCoupling;
        route.amount = 0.5f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        // applyTriangularCouplingInput must have been called.
        reportTest("TriangularCoupling: applyTriangularCouplingInput called on dest",
                   dst.triangularCouplingReceived);
        // Source I/P/C must be passed intact to the destination.
        reportTest("TriangularCoupling: source I state forwarded correctly",
                   std::abs(dst.lastTriangularState.I - 0.8f) < 0.001f);
        reportTest("TriangularCoupling: source P state forwarded correctly",
                   std::abs(dst.lastTriangularState.P - 0.6f) < 0.001f);
        reportTest("TriangularCoupling: source C state forwarded correctly",
                   std::abs(dst.lastTriangularState.C - 0.4f) < 0.001f);
        reportTest("TriangularCoupling: route amount forwarded correctly",
                   std::abs(dst.lastTriangularAmount - 0.5f) < 0.001f);
    }

    // Default fallback (non-Oxytocin dest): applyTriangularCouplingInput must
    // re-enter applyCouplingInput with AmpToFilter semantics (fixes #434).
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        OxytocinLikeTestEngine src;
        TestEngine dst("GenericDst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.setLoveState(0.7f, 0.5f, 0.3f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::TriangularCoupling;
        route.amount = 1.0f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        // Reset so we can observe only the fallback-triggered applyCouplingInput call.
        dst.couplingInputReceived = false;

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        // The default SynthEngine::applyTriangularCouplingInput must have forwarded
        // a signal via applyCouplingInput(AmpToFilter, ...) — proving non-Oxytocin
        // engines receive audible modulation and are not silently dropped (#434).
        reportTest("TriangularCoupling fallback: non-Oxytocin dest gets AmpToFilter",
                   dst.couplingInputReceived);
        reportTest("TriangularCoupling fallback: forwarded as AmpToFilter type",
                   dst.lastCouplingType == CouplingType::AmpToFilter);
    }

    // Non-Oxytocin source: getLoveTriangleState returns {0,0,0}.
    // TriangularCoupling from a non-love-triangle engine must still call
    // applyTriangularCouplingInput (not silently skip), but with zero I/P/C.
    {
        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine src("GenericSrc");  // getLoveTriangleState returns {0,0,0}
        TestEngine dst("Dst");
        src.prepare(44100.0, blockSize);
        dst.prepare(44100.0, blockSize);
        src.fillOutput(0.5f);

        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = { &src, &dst, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        MegaCouplingMatrix::CouplingRoute route;
        route.sourceSlot = 0;
        route.destSlot = 1;
        route.type = CouplingType::TriangularCoupling;
        route.amount = 0.8f;
        route.isNormalled = false;
        route.active = true;
        matrix.addRoute(route);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(blockSize, routes);

        // applyTriangularCouplingInput still called (not skipped).
        reportTest("TriangularCoupling: non-Oxytocin source still calls dest handler",
                   dst.triangularCouplingReceived);
        // State is {0,0,0} because generic sources have no love state.
        reportTest("TriangularCoupling: non-Oxytocin source yields zero I/P/C",
                   dst.lastTriangularState.I == 0.0f
                   && dst.lastTriangularState.P == 0.0f
                   && dst.lastTriangularState.C == 0.0f);
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
    testKnotTopology();
    testTriangularCoupling();

    std::cout << "\n  Coupling Tests: " << g_couplingTestsPassed << " passed, "
              << g_couplingTestsFailed << " failed\n";

    return g_couplingTestsFailed;
}

} // namespace coupling_tests
