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

// ============================================================================
// Helpers
// ============================================================================

// Build a matrix, wired with two prepared TestEngines, and a single active
// route using `type` and `amount`.  Returns (by populating the out-params)
// so the caller owns the engines and the matrix for further assertions.
static void runSingleRouteBlock(MegaCouplingMatrix& matrix,
                                TestEngine& src,
                                TestEngine& dst,
                                CouplingType type,
                                float amount,
                                int blockSize = 256)
{
    matrix.prepare(blockSize);
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.fillOutput(0.5f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines =
        { &src, &dst, nullptr, nullptr, nullptr };
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot  = 0;
    route.destSlot    = 1;
    route.type        = type;
    route.amount      = amount;
    route.isNormalled = false;
    route.active      = true;
    matrix.addRoute(route);

    auto routes = matrix.loadRoutes();
    matrix.processBlock(blockSize, routes);
}

// ============================================================================
// Control-rate coupling types (non-audio-rate) — all routed through the
// fillControlRateBuffer → applyCouplingInput path.  The test verifies that:
//   1. applyCouplingInput is called on the destination engine.
//   2. The CouplingType tag passed is exactly the type we configured.
//   3. The amount passed matches the route amount (±0.001 tolerance).
//   4. numSamples equals blockSize.
// ============================================================================

static void testControlRateCouplingTypes()
{
    std::cout << "\n--- Control-Rate Coupling Types ---\n";
    constexpr int blockSize = 256;

    // Table of all control-rate types (non-audio-rate, non-special-path types).
    // KnotTopology and TriangularCoupling are exercised in their own sections.
    // AudioToFM, AudioToRing, AudioToWavetable, AudioToBuffer use the audio-rate
    // path and are covered in testAudioRateCouplingTypes().
    const struct { CouplingType type; const char* name; } kControlRateTypes[] = {
        { CouplingType::AmpToPitch,    "AmpToPitch"    },
        { CouplingType::LFOToPitch,    "LFOToPitch"    },
        { CouplingType::EnvToMorph,    "EnvToMorph"    },
        { CouplingType::FilterToFilter,"FilterToFilter" },
        { CouplingType::AmpToChoke,    "AmpToChoke"    },
        { CouplingType::RhythmToBlend, "RhythmToBlend" },
        { CouplingType::EnvToDecay,    "EnvToDecay"    },
        { CouplingType::PitchToPitch,  "PitchToPitch"  },
    };

    for (const auto& entry : kControlRateTypes)
    {
        MegaCouplingMatrix matrix;
        TestEngine src("Src"), dst("Dst");
        runSingleRouteBlock(matrix, src, dst, entry.type, 0.6f, blockSize);

        // Verify delivery
        {
            std::string label = std::string(entry.name) + ": applyCouplingInput called";
            reportTest(label.c_str(), dst.couplingInputReceived);
        }
        {
            std::string label = std::string(entry.name) + ": correct CouplingType tag";
            reportTest(label.c_str(), dst.lastCouplingType == entry.type);
        }
        {
            std::string label = std::string(entry.name) + ": correct amount (0.6)";
            reportTest(label.c_str(), std::abs(dst.lastCouplingAmount - 0.6f) < 0.001f);
        }
        {
            std::string label = std::string(entry.name) + ": numSamples == blockSize";
            reportTest(label.c_str(), dst.lastCouplingNumSamples == blockSize);
        }
    }
}

// ============================================================================
// Audio-rate coupling types — routed through the per-sample L/R stereo mixdown
// path.  AudioToBuffer uses the processAudioRoute (sink) path and is tested
// separately.
// ============================================================================

static void testAudioRateCouplingTypes()
{
    std::cout << "\n--- Audio-Rate Coupling Types (AudioToFM, AudioToRing, AudioToWavetable) ---\n";
    constexpr int blockSize = 256;

    const struct { CouplingType type; const char* name; } kAudioRateTypes[] = {
        { CouplingType::AudioToFM,         "AudioToFM"         },
        { CouplingType::AudioToRing,        "AudioToRing"       },
        { CouplingType::AudioToWavetable,   "AudioToWavetable"  },
    };

    for (const auto& entry : kAudioRateTypes)
    {
        MegaCouplingMatrix matrix;
        TestEngine src("Src"), dst("Dst");
        runSingleRouteBlock(matrix, src, dst, entry.type, 0.5f, blockSize);

        {
            std::string label = std::string(entry.name) + ": applyCouplingInput called";
            reportTest(label.c_str(), dst.couplingInputReceived);
        }
        {
            std::string label = std::string(entry.name) + ": correct CouplingType tag";
            reportTest(label.c_str(), dst.lastCouplingType == entry.type);
        }
        {
            std::string label = std::string(entry.name) + ": amount forwarded correctly";
            reportTest(label.c_str(), std::abs(dst.lastCouplingAmount - 0.5f) < 0.001f);
        }
        {
            std::string label = std::string(entry.name) + ": numSamples == blockSize";
            reportTest(label.c_str(), dst.lastCouplingNumSamples == blockSize);
        }
    }

    // Verify audio-rate cycle detection blocks A→B→A routes.
    {
        std::cout << "\n--- Audio-Rate Cycle Detection ---\n";

        MegaCouplingMatrix matrix;
        matrix.prepare(blockSize);

        TestEngine a("A"), b("B");
        a.prepare(44100.0, blockSize);
        b.prepare(44100.0, blockSize);
        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines =
            { &a, &b, nullptr, nullptr, nullptr };
        matrix.setEngines(engines);

        // Add A→B (AudioToFM).
        MegaCouplingMatrix::CouplingRoute routeAB;
        routeAB.sourceSlot  = 0;
        routeAB.destSlot    = 1;
        routeAB.type        = CouplingType::AudioToFM;
        routeAB.amount      = 0.5f;
        routeAB.isNormalled = false;
        routeAB.active      = true;
        matrix.addRoute(routeAB);

        // Attempt to add B→A (same audio-rate type) — would create a cycle.
        MegaCouplingMatrix::CouplingRoute routeBA;
        routeBA.sourceSlot  = 1;
        routeBA.destSlot    = 0;
        routeBA.type        = CouplingType::AudioToFM;
        routeBA.amount      = 0.5f;
        routeBA.isNormalled = false;
        routeBA.active      = true;
        matrix.addRoute(routeBA);

        // Only one route should have been admitted (the second is a cycle).
        reportTest("AudioToFM A→B→A cycle blocked: only 1 route admitted",
                   matrix.getRoutes().size() == 1);

        // wouldCreateCycle must return true for the reverse direction.
        reportTest("wouldCreateCycle returns true for B→A after A→B",
                   matrix.wouldCreateCycle(1, 0, CouplingType::AudioToFM));

        // Control-rate types must NOT be blocked (they're latent, not audio-rate).
        reportTest("wouldCreateCycle returns false for control-rate AmpToFilter",
                   !matrix.wouldCreateCycle(1, 0, CouplingType::AmpToFilter));
    }
}

// ============================================================================
// AudioToBuffer — uses the processAudioRoute / sink path.
// When the destination does NOT implement IAudioBufferSink the sinkCache will
// be nullptr and processBlock must skip the route without crashing.
// ============================================================================

static void testAudioToBufferNonSinkDest()
{
    std::cout << "\n--- AudioToBuffer (non-sink destination) ---\n";
    constexpr int blockSize = 256;

    MegaCouplingMatrix matrix;
    TestEngine src("Src"), dst("Dst");   // TestEngine does not implement IAudioBufferSink
    matrix.prepare(blockSize);
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.fillOutput(0.5f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines =
        { &src, &dst, nullptr, nullptr, nullptr };
    matrix.setEngines(engines);

    // resolveAudioToBufferSinks is called inside addRoute for AudioToBuffer.
    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot  = 0;
    route.destSlot    = 1;
    route.type        = CouplingType::AudioToBuffer;
    route.amount      = 0.8f;
    route.isNormalled = false;
    route.active      = true;
    matrix.addRoute(route);

    // sinkCache should be nullptr (TestEngine doesn't implement IAudioBufferSink).
    auto routes = matrix.getRoutes();
    bool sinkIsNull = !routes.empty() && (routes[0].sinkCache == nullptr);
    reportTest("AudioToBuffer: sinkCache is nullptr for non-sink dest", sinkIsNull);

    // processBlock must not crash and must NOT call applyCouplingInput.
    auto liveRoutes = matrix.loadRoutes();
    matrix.processBlock(blockSize, liveRoutes);
    reportTest("AudioToBuffer: non-sink dest does not crash processBlock", true);
    // The sink path does not call applyCouplingInput — it writes to the ring buffer only.
    // Since sinkCache is null the route is skipped entirely, so dst receives nothing.
    reportTest("AudioToBuffer: applyCouplingInput NOT called when sink is null",
               !dst.couplingInputReceived);
}

// ============================================================================
// AmpToChoke priority ordering
//
// AmpToChoke is semantically different from simple filter modulation: a loud
// source is supposed to attenuate ("choke") the destination.  The test verifies
// that when two routes both target the same destination — one AmpToChoke and one
// AmpToFilter — both are delivered independently (no route suppresses the other).
// ============================================================================

static void testAmpToChokePriorityOrdering()
{
    std::cout << "\n--- AmpToChoke Priority / Ordering ---\n";
    constexpr int blockSize = 256;

    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine src1("Src1"), src2("Src2"), dst("Dst");
    src1.prepare(44100.0, blockSize);
    src2.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src1.fillOutput(0.9f);  // loud — should trigger choke
    src2.fillOutput(0.3f);  // moderate

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines =
        { &src1, &src2, &dst, nullptr, nullptr };
    matrix.setEngines(engines);

    // Route 1: src1 → dst via AmpToChoke (slot 0 → slot 2)
    MegaCouplingMatrix::CouplingRoute chokeRoute;
    chokeRoute.sourceSlot  = 0;
    chokeRoute.destSlot    = 2;
    chokeRoute.type        = CouplingType::AmpToChoke;
    chokeRoute.amount      = 0.8f;
    chokeRoute.isNormalled = false;
    chokeRoute.active      = true;
    matrix.addRoute(chokeRoute);

    // Route 2: src2 → dst via AmpToFilter (slot 1 → slot 2)
    MegaCouplingMatrix::CouplingRoute filterRoute;
    filterRoute.sourceSlot  = 1;
    filterRoute.destSlot    = 2;
    filterRoute.type        = CouplingType::AmpToFilter;
    filterRoute.amount      = 0.4f;
    filterRoute.isNormalled = false;
    filterRoute.active      = true;
    matrix.addRoute(filterRoute);

    reportTest("AmpToChoke ordering: both routes admitted",
               matrix.getRoutes().size() == 2);

    auto routes = matrix.loadRoutes();
    matrix.processBlock(blockSize, routes);

    // dst.applyCouplingInput is called at least once (could be AmpToChoke or
    // AmpToFilter depending on route ordering — either is correct).
    reportTest("AmpToChoke ordering: dst received at least one coupling input",
               dst.couplingInputReceived);

    // The last recorded type must be one of the two we added (not some other type).
    bool lastTypeIsKnown = (dst.lastCouplingType == CouplingType::AmpToChoke
                         || dst.lastCouplingType == CouplingType::AmpToFilter);
    reportTest("AmpToChoke ordering: last coupling type is AmpToChoke or AmpToFilter",
               lastTypeIsKnown);
}

// ============================================================================
// Zero-amount edge case for all coupling types
// Route amount = 0.0005f (below the 0.001f threshold) → route treated as
// inactive, applyCouplingInput must NOT be called.
// ============================================================================

static void testZeroAmountAllTypes()
{
    std::cout << "\n--- Zero Amount (below threshold) for Each Coupling Type ---\n";
    constexpr int blockSize = 256;

    const CouplingType allTypes[] = {
        CouplingType::AmpToFilter,
        CouplingType::AmpToPitch,
        CouplingType::LFOToPitch,
        CouplingType::EnvToMorph,
        CouplingType::AudioToFM,
        CouplingType::AudioToRing,
        CouplingType::FilterToFilter,
        CouplingType::AmpToChoke,
        CouplingType::RhythmToBlend,
        CouplingType::EnvToDecay,
        CouplingType::PitchToPitch,
        CouplingType::AudioToWavetable,
        // AudioToBuffer: skip — it uses sinkCache; null sink already skips it.
        CouplingType::KnotTopology,
        CouplingType::TriangularCoupling,
    };

    for (const auto type : allTypes)
    {
        MegaCouplingMatrix matrix;
        TestEngine src("Src"), dst("Dst");
        // Use amount=0.0005f (below threshold=0.001f)
        runSingleRouteBlock(matrix, src, dst, type, 0.0005f, blockSize);

        // TriangularCoupling and KnotTopology use separate dispatch paths but
        // are still subject to the 0.001f amount guard in processBlock.
        reportTest("Zero amount → no coupling delivered (type skipped)",
                   !dst.couplingInputReceived && !dst.triangularCouplingReceived);
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
    testControlRateCouplingTypes();
    testAudioRateCouplingTypes();
    testAudioToBufferNonSinkDest();
    testAmpToChokePriorityOrdering();
    testZeroAmountAllTypes();

    std::cout << "\n  Coupling Tests: " << g_couplingTestsPassed << " passed, "
              << g_couplingTestsFailed << " failed\n";

    return g_couplingTestsFailed;
}

} // namespace coupling_tests
