/*
    XOlokun Coupling Matrix Tests
    ===============================
    Tests for MegaCouplingMatrix route management and processing.
    Migrated to Catch2 v3: issue #81
*/

#include "CouplingMatrixTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/SynthEngine.h"
#include "Core/MegaCouplingMatrix.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <string>
#include <vector>
#include <array>
#include <cmath>

using namespace xoceanus;

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

    void renderBlock(juce::AudioBuffer<float>& /*buffer*/, juce::MidiBuffer& /*midi*/, int /*numSamples*/) override {}

    float getSampleForCoupling(int /*channel*/, int sampleIndex) const override
    {
        auto idx = static_cast<size_t>(sampleIndex);
        if (idx < outputBuffer.size())
            return outputBuffer[idx];
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int numSamples) override
    {
        couplingInputReceived = true;
        lastCouplingType = type;
        lastCouplingAmount = amount;
        lastCouplingNumSamples = numSamples;
    }

    void applyTriangularCouplingInput(LoveTriangleState state, float amount) override
    {
        triangularCouplingReceived = true;
        lastTriangularState = state;
        lastTriangularAmount = amount;
        SynthEngine::applyTriangularCouplingInput(state, amount);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override { return {}; }
    void attachParameters(juce::AudioProcessorValueTreeState& /*apvts*/) override {}
    juce::String getEngineId() const override { return engineId; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFFFFFFF); }
    int getMaxVoices() const override { return 1; }

    void fillOutput(float value) { std::fill(outputBuffer.begin(), outputBuffer.end(), value); }

    bool couplingInputReceived = false;
    CouplingType lastCouplingType = CouplingType::AmpToFilter;
    float lastCouplingAmount = 0.0f;
    int lastCouplingNumSamples = 0;

    bool triangularCouplingReceived = false;
    LoveTriangleState lastTriangularState{};
    float lastTriangularAmount = 0.0f;

private:
    juce::String engineId;
    std::vector<float> outputBuffer;
};

class OxytocinLikeTestEngine : public TestEngine
{
public:
    OxytocinLikeTestEngine() : TestEngine("OxytocinLike") {}
    void setLoveState(float I, float P, float C) { loveState = {I, P, C}; }
    LoveTriangleState getLoveTriangleState() const override { return loveState; }

private:
    LoveTriangleState loveState{0.7f, 0.5f, 0.3f};
};

// Helper: build matrix + two prepared TestEngines with a single route.
static void runSingleRouteBlock(MegaCouplingMatrix& matrix, TestEngine& src, TestEngine& dst, CouplingType type,
                                float amount, int blockSize = 256)
{
    matrix.prepare(blockSize);
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.fillOutput(0.5f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot = 0;
    route.destSlot = 1;
    route.type = type;
    route.amount = amount;
    route.isNormalled = false;
    route.active = true;
    matrix.addRoute(route);

    auto routes = matrix.loadRoutes();
    matrix.processBlock(blockSize, routes);
}

//==============================================================================
// Prepare
//==============================================================================

TEST_CASE("CouplingMatrix - prepare initializes without crash", "[coupling]")
{
    MegaCouplingMatrix matrix;
    CHECK_NOTHROW(matrix.prepare(512));
}

//==============================================================================
// Route management
//==============================================================================

TEST_CASE("CouplingMatrix - addRoute: single route appears in getRoutes", "[coupling][routes]")
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

    CHECK(matrix.getRoutes().size() == 1);
}

TEST_CASE("CouplingMatrix - addRoute: multiple routes added correctly", "[coupling][routes]")
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
    CHECK(matrix.getRoutes().size() == 3);
}

TEST_CASE("CouplingMatrix - removeUserRoute removes user route and re-enables normalled", "[coupling][routes]")
{
    MegaCouplingMatrix matrix;
    matrix.prepare(512);

    MegaCouplingMatrix::CouplingRoute normalled;
    normalled.sourceSlot = 0;
    normalled.destSlot = 1;
    normalled.type = CouplingType::AmpToFilter;
    normalled.amount = 0.3f;
    normalled.isNormalled = true;
    normalled.active = false;
    matrix.addRoute(normalled);

    MegaCouplingMatrix::CouplingRoute userRoute;
    userRoute.sourceSlot = 0;
    userRoute.destSlot = 1;
    userRoute.type = CouplingType::AmpToFilter;
    userRoute.amount = 0.8f;
    userRoute.isNormalled = false;
    userRoute.active = true;
    matrix.addRoute(userRoute);

    matrix.removeUserRoute(0, 1, CouplingType::AmpToFilter);

    auto routes = matrix.getRoutes();
    bool hasNormalled = false, hasUser = false;
    bool normalledActive = false;
    for (const auto& r : routes)
    {
        if (r.isNormalled)
        {
            hasNormalled = true;
            normalledActive = r.active;
        }
        if (!r.isNormalled)
            hasUser = true;
    }
    CHECK(!hasUser);
    CHECK(hasNormalled);
    CHECK(normalledActive);
}

TEST_CASE("CouplingMatrix - clearRoutes empties all routes", "[coupling][routes]")
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
    CHECK(matrix.getRoutes().empty());
}

//==============================================================================
// processBlock
//==============================================================================

TEST_CASE("CouplingMatrix - processBlock with no routes does not call applyCouplingInput", "[coupling][process]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine src("Src"), dst("Dst");
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    auto routes = matrix.loadRoutes();
    CHECK_NOTHROW(matrix.processBlock(blockSize, routes));
    CHECK(!dst.couplingInputReceived);
}

TEST_CASE("CouplingMatrix - active route calls applyCouplingInput with correct args", "[coupling][process]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine src("Src"), dst("Dst");
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.fillOutput(0.5f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
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

    CHECK(dst.couplingInputReceived);
    CHECK(dst.lastCouplingType == CouplingType::AmpToFilter);
    CHECK(std::abs(dst.lastCouplingAmount - 0.7f) < 0.001f);
    CHECK(dst.lastCouplingNumSamples == blockSize);
}

TEST_CASE("CouplingMatrix - route amount < 0.001 treated as inactive", "[coupling][process]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    TestEngine src("Src"), dst("Dst");
    runSingleRouteBlock(matrix, src, dst, CouplingType::AmpToFilter, 0.0005f, blockSize);
    CHECK(!dst.couplingInputReceived);
}

TEST_CASE("CouplingMatrix - out-of-bounds slot indices do not crash", "[coupling][process]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine eng("Eng");
    eng.prepare(44100.0, blockSize);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&eng, nullptr, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot = -1;
    route.destSlot = 99;
    route.type = CouplingType::AmpToFilter;
    route.amount = 0.5f;
    route.isNormalled = false;
    route.active = true;
    matrix.addRoute(route);

    auto routes = matrix.loadRoutes();
    CHECK_NOTHROW(matrix.processBlock(blockSize, routes));
}

TEST_CASE("CouplingMatrix - null engine pointers in slots do not crash", "[coupling][process]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {nullptr, nullptr, nullptr, nullptr, nullptr};
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
    CHECK_NOTHROW(matrix.processBlock(blockSize, routes));
}

//==============================================================================
// KnotTopology
//==============================================================================

TEST_CASE("KnotTopology - bidirectional: both engines receive coupling", "[coupling][knot]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine src("Src"), dst("Dst");
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.fillOutput(0.5f);
    dst.fillOutput(0.3f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
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

    CHECK(dst.couplingInputReceived);
    CHECK(src.couplingInputReceived);
    CHECK(dst.lastCouplingType == CouplingType::AmpToFilter);
    CHECK(src.lastCouplingType == CouplingType::AmpToFilter);
}

TEST_CASE("KnotTopology - self-route is blocked by addRoute", "[coupling][knot]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine eng("Self");
    eng.prepare(44100.0, blockSize);
    eng.fillOutput(0.5f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&eng, nullptr, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute selfRoute;
    selfRoute.sourceSlot = 0;
    selfRoute.destSlot = 0;
    selfRoute.type = CouplingType::KnotTopology;
    selfRoute.amount = 0.5f;
    selfRoute.isNormalled = false;
    selfRoute.active = true;
    matrix.addRoute(selfRoute);

    CHECK(matrix.getRoutes().empty());
}

TEST_CASE("KnotTopology - linking number 5 (amount=1.0) yields scaledAmount=1.0", "[coupling][knot]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    TestEngine src("Src"), dst("Dst");
    runSingleRouteBlock(matrix, src, dst, CouplingType::KnotTopology, 1.0f, blockSize);
    CHECK(std::abs(dst.lastCouplingAmount - 1.0f) < 0.01f);
}

//==============================================================================
// TriangularCoupling
//==============================================================================

TEST_CASE("TriangularCoupling - applyTriangularCouplingInput called with correct I/P/C", "[coupling][triangular]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    OxytocinLikeTestEngine src;
    TestEngine dst("Dst");
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.setLoveState(0.8f, 0.6f, 0.4f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
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

    CHECK(dst.triangularCouplingReceived);
    CHECK(std::abs(dst.lastTriangularState.I - 0.8f) < 0.001f);
    CHECK(std::abs(dst.lastTriangularState.P - 0.6f) < 0.001f);
    CHECK(std::abs(dst.lastTriangularState.C - 0.4f) < 0.001f);
    CHECK(std::abs(dst.lastTriangularAmount - 0.5f) < 0.001f);
}

TEST_CASE("TriangularCoupling - fallback: non-Oxytocin dest gets AmpToFilter", "[coupling][triangular]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    OxytocinLikeTestEngine src;
    TestEngine dst("GenericDst");
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.setLoveState(0.7f, 0.5f, 0.3f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot = 0;
    route.destSlot = 1;
    route.type = CouplingType::TriangularCoupling;
    route.amount = 1.0f;
    route.isNormalled = false;
    route.active = true;
    matrix.addRoute(route);

    dst.couplingInputReceived = false;

    auto routes = matrix.loadRoutes();
    matrix.processBlock(blockSize, routes);

    CHECK(dst.couplingInputReceived);
    CHECK(dst.lastCouplingType == CouplingType::AmpToFilter);
}

TEST_CASE("TriangularCoupling - non-Oxytocin source still calls dest handler with zero I/P/C", "[coupling][triangular]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    TestEngine src("GenericSrc"), dst("Dst");
    runSingleRouteBlock(matrix, src, dst, CouplingType::TriangularCoupling, 0.8f, blockSize);

    CHECK(dst.triangularCouplingReceived);
    CHECK(dst.lastTriangularState.I == 0.0f);
    CHECK(dst.lastTriangularState.P == 0.0f);
    CHECK(dst.lastTriangularState.C == 0.0f);
}

//==============================================================================
// Control-rate coupling types
//==============================================================================

TEST_CASE("Control-rate coupling types - all deliver correct type/amount/numSamples", "[coupling][control-rate]")
{
    constexpr int blockSize = 256;

    const struct
    {
        CouplingType type;
        const char* name;
    } kTypes[] = {
        {CouplingType::AmpToPitch, "AmpToPitch"}, {CouplingType::LFOToPitch, "LFOToPitch"},
        {CouplingType::EnvToMorph, "EnvToMorph"}, {CouplingType::FilterToFilter, "FilterToFilter"},
        {CouplingType::AmpToChoke, "AmpToChoke"}, {CouplingType::RhythmToBlend, "RhythmToBlend"},
        {CouplingType::EnvToDecay, "EnvToDecay"}, {CouplingType::PitchToPitch, "PitchToPitch"},
    };

    for (const auto& entry : kTypes)
    {
        MegaCouplingMatrix matrix;
        TestEngine src("Src"), dst("Dst");
        runSingleRouteBlock(matrix, src, dst, entry.type, 0.6f, blockSize);

        INFO("Type: " << entry.name);
        CHECK(dst.couplingInputReceived);
        CHECK(dst.lastCouplingType == entry.type);
        CHECK(std::abs(dst.lastCouplingAmount - 0.6f) < 0.001f);
        CHECK(dst.lastCouplingNumSamples == blockSize);
    }
}

//==============================================================================
// Audio-rate coupling types
//==============================================================================

TEST_CASE("Audio-rate coupling types - AudioToFM/AudioToRing/AudioToWavetable deliver correctly",
          "[coupling][audio-rate]")
{
    constexpr int blockSize = 256;

    const struct
    {
        CouplingType type;
        const char* name;
    } kTypes[] = {
        {CouplingType::AudioToFM, "AudioToFM"},
        {CouplingType::AudioToRing, "AudioToRing"},
        {CouplingType::AudioToWavetable, "AudioToWavetable"},
    };

    for (const auto& entry : kTypes)
    {
        MegaCouplingMatrix matrix;
        TestEngine src("Src"), dst("Dst");
        runSingleRouteBlock(matrix, src, dst, entry.type, 0.5f, blockSize);

        INFO("Type: " << entry.name);
        CHECK(dst.couplingInputReceived);
        CHECK(dst.lastCouplingType == entry.type);
        CHECK(std::abs(dst.lastCouplingAmount - 0.5f) < 0.001f);
        CHECK(dst.lastCouplingNumSamples == blockSize);
    }
}

TEST_CASE("Audio-rate cycle detection - A→B→A AudioToFM cycle is blocked", "[coupling][audio-rate][cycle]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine a("A"), b("B");
    a.prepare(44100.0, blockSize);
    b.prepare(44100.0, blockSize);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&a, &b, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute routeAB;
    routeAB.sourceSlot = 0;
    routeAB.destSlot = 1;
    routeAB.type = CouplingType::AudioToFM;
    routeAB.amount = 0.5f;
    routeAB.isNormalled = false;
    routeAB.active = true;
    matrix.addRoute(routeAB);

    MegaCouplingMatrix::CouplingRoute routeBA;
    routeBA.sourceSlot = 1;
    routeBA.destSlot = 0;
    routeBA.type = CouplingType::AudioToFM;
    routeBA.amount = 0.5f;
    routeBA.isNormalled = false;
    routeBA.active = true;
    matrix.addRoute(routeBA);

    CHECK(matrix.getRoutes().size() == 1);
    CHECK(matrix.wouldCreateCycle(1, 0, CouplingType::AudioToFM));
    CHECK(!matrix.wouldCreateCycle(1, 0, CouplingType::AmpToFilter));
}

//==============================================================================
// AudioToBuffer - non-sink destination
//==============================================================================

TEST_CASE("AudioToBuffer - sinkCache is nullptr for non-sink dest, no crash", "[coupling][audio-buffer]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    TestEngine src("Src"), dst("Dst");
    matrix.prepare(blockSize);
    src.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src.fillOutput(0.5f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src, &dst, nullptr, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot = 0;
    route.destSlot = 1;
    route.type = CouplingType::AudioToBuffer;
    route.amount = 0.8f;
    route.isNormalled = false;
    route.active = true;
    matrix.addRoute(route);

    auto routes = matrix.getRoutes();
    bool sinkIsNull = !routes.empty() && (routes[0].sinkCache == nullptr);
    CHECK(sinkIsNull);

    auto liveRoutes = matrix.loadRoutes();
    CHECK_NOTHROW(matrix.processBlock(blockSize, liveRoutes));
    CHECK(!dst.couplingInputReceived);
}

//==============================================================================
// AmpToChoke priority ordering
//==============================================================================

TEST_CASE("AmpToChoke - two routes to same dest are both admitted and delivered", "[coupling][choke]")
{
    constexpr int blockSize = 256;
    MegaCouplingMatrix matrix;
    matrix.prepare(blockSize);

    TestEngine src1("Src1"), src2("Src2"), dst("Dst");
    src1.prepare(44100.0, blockSize);
    src2.prepare(44100.0, blockSize);
    dst.prepare(44100.0, blockSize);
    src1.fillOutput(0.9f);
    src2.fillOutput(0.3f);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> engines = {&src1, &src2, &dst, nullptr, nullptr};
    matrix.setEngines(engines);

    MegaCouplingMatrix::CouplingRoute chokeRoute;
    chokeRoute.sourceSlot = 0;
    chokeRoute.destSlot = 2;
    chokeRoute.type = CouplingType::AmpToChoke;
    chokeRoute.amount = 0.8f;
    chokeRoute.isNormalled = false;
    chokeRoute.active = true;
    matrix.addRoute(chokeRoute);

    MegaCouplingMatrix::CouplingRoute filterRoute;
    filterRoute.sourceSlot = 1;
    filterRoute.destSlot = 2;
    filterRoute.type = CouplingType::AmpToFilter;
    filterRoute.amount = 0.4f;
    filterRoute.isNormalled = false;
    filterRoute.active = true;
    matrix.addRoute(filterRoute);

    CHECK(matrix.getRoutes().size() == 2);

    auto routes = matrix.loadRoutes();
    matrix.processBlock(blockSize, routes);

    CHECK(dst.couplingInputReceived);
    bool lastTypeIsKnown =
        (dst.lastCouplingType == CouplingType::AmpToChoke || dst.lastCouplingType == CouplingType::AmpToFilter);
    CHECK(lastTypeIsKnown);
}

//==============================================================================
// Zero-amount edge case for all coupling types
//==============================================================================

TEST_CASE("Zero amount (below 0.001 threshold) - no coupling delivered for any type", "[coupling][zero-amount]")
{
    constexpr int blockSize = 256;

    const CouplingType allTypes[] = {
        CouplingType::AmpToFilter,    CouplingType::AmpToPitch,         CouplingType::LFOToPitch,
        CouplingType::EnvToMorph,     CouplingType::AudioToFM,          CouplingType::AudioToRing,
        CouplingType::FilterToFilter, CouplingType::AmpToChoke,         CouplingType::RhythmToBlend,
        CouplingType::EnvToDecay,     CouplingType::PitchToPitch,       CouplingType::AudioToWavetable,
        CouplingType::KnotTopology,   CouplingType::TriangularCoupling,
    };

    for (const auto type : allTypes)
    {
        MegaCouplingMatrix matrix;
        TestEngine src("Src"), dst("Dst");
        runSingleRouteBlock(matrix, src, dst, type, 0.0005f, blockSize);

        INFO("CouplingType index " << static_cast<int>(type));
        CHECK(!dst.couplingInputReceived);
        CHECK(!dst.triangularCouplingReceived);
    }
}

// Backward-compat shim
namespace coupling_tests
{
int runAll()
{
    return 0;
}
} // namespace coupling_tests
