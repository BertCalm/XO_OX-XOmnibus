/*
    XOceanus Full Pipeline Integration Tests
    ==========================================
    Smoke tests for the complete render pipeline.
    Covers issue #456: "No integration test for full render pipeline".
    Migrated to Catch2 v3: issue #81
*/

#include "FullPipelineTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Core/SynthEngine.h"
#include "Core/MegaCouplingMatrix.h"
#include "Engines/OddfeliX/OddfeliXEngine.h"
#include "Engines/Bob/BobEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <array>
#include <memory>

using namespace xoceanus;

//==============================================================================
// Helpers
//==============================================================================

static bool allFinite(const juce::AudioBuffer<float>& buf, int numSamples)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            if (!std::isfinite(data[i]))
                return false;
    }
    return true;
}

static float rmsOf(const juce::AudioBuffer<float>& buf, int numSamples)
{
    double sum = 0.0;
    int total = 0;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            sum += static_cast<double>(data[i]) * static_cast<double>(data[i]);
            ++total;
        }
    }
    return (total > 0) ? static_cast<float>(std::sqrt(sum / total)) : 0.0f;
}

static juce::MidiBuffer makeNoteOn(int channel, int note, int velocity)
{
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(channel, note, static_cast<uint8_t>(velocity)), 0);
    return midi;
}

static juce::MidiBuffer makeNoteOff(int channel, int note)
{
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOff(channel, note), 0);
    return midi;
}

//==============================================================================
// Test 1 — Single engine render stability (1000 blocks)
//==============================================================================

TEST_CASE("Pipeline - SnapEngine no NaN/Inf in 1000 blocks", "[pipeline][stability]")
{
    constexpr double kSampleRate = 44100.0;
    constexpr int kBlockSize = 512;
    constexpr int kNumBlocks = 1000;

    auto engine = std::make_unique<SnapEngine>();
    engine->prepare(kSampleRate, kBlockSize);
    engine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    engine->reset();

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    bool anyFiniteFailure = false;
    bool receivedAudio = false;

    {
        buffer.clear();
        juce::MidiBuffer noteOn = makeNoteOn(1, 60, 100);
        engine->wakeSilenceGate();
        engine->renderBlock(buffer, noteOn, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
            anyFiniteFailure = true;
        if (rmsOf(buffer, kBlockSize) > 1e-6f)
            receivedAudio = true;
    }

    for (int b = 1; b < kNumBlocks && !anyFiniteFailure; ++b)
    {
        buffer.clear();
        juce::MidiBuffer emptyMidi;
        engine->renderBlock(buffer, emptyMidi, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }
        if (rmsOf(buffer, kBlockSize) > 1e-6f)
            receivedAudio = true;
    }

    CHECK(!anyFiniteFailure);
    CHECK(receivedAudio);
}

//==============================================================================
// Test 2 — Two engines coupled via AmpToFilter (1000 blocks)
//==============================================================================

TEST_CASE("Pipeline - Snap+Bob AmpToFilter coupling no NaN/Inf in 1000 blocks", "[pipeline][coupling]")
{
    constexpr double kSampleRate = 44100.0;
    constexpr int kBlockSize = 512;
    constexpr int kNumBlocks = 1000;

    auto snapEngine = std::make_unique<SnapEngine>();
    auto bobEngine = std::make_unique<BobEngine>();

    snapEngine->prepare(kSampleRate, kBlockSize);
    snapEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    snapEngine->reset();
    bobEngine->prepare(kSampleRate, kBlockSize);
    bobEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    bobEngine->reset();

    MegaCouplingMatrix matrix;
    matrix.prepare(kBlockSize, kSampleRate);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs = {snapEngine.get(), bobEngine.get(), nullptr,
                                                                         nullptr, nullptr};
    matrix.setEngines(enginePtrs);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot = 0;
    route.destSlot = 1;
    route.type = CouplingType::AmpToFilter;
    route.amount = 0.5f;
    route.isNormalled = false;
    route.active = true;
    matrix.addRoute(route);

    CHECK(matrix.getRoutes().size() == 1);

    juce::AudioBuffer<float> snapBuf(2, kBlockSize);
    juce::AudioBuffer<float> bobBuf(2, kBlockSize);
    juce::AudioBuffer<float> mixBuf(2, kBlockSize);
    bool anyFiniteFailure = false;
    bool receivedAudio = false;

    for (int b = 0; b < kNumBlocks && !anyFiniteFailure; ++b)
    {
        snapBuf.clear();
        bobBuf.clear();
        mixBuf.clear();

        juce::MidiBuffer midiBlock;
        if (b == 0)
        {
            midiBlock = makeNoteOn(1, 60, 100);
            snapEngine->wakeSilenceGate();
            bobEngine->wakeSilenceGate();
        }

        snapEngine->renderBlock(snapBuf, midiBlock, kBlockSize);
        auto routes = matrix.loadRoutes();
        matrix.processBlock(kBlockSize, routes);
        juce::MidiBuffer emptyMidi;
        bobEngine->renderBlock(bobBuf, emptyMidi, kBlockSize);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* sd = snapBuf.getReadPointer(ch);
            const float* bd = bobBuf.getReadPointer(ch);
            float* md = mixBuf.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                md[i] = (sd[i] + bd[i]) * 0.5f;
        }

        if (!allFinite(mixBuf, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }
        if (rmsOf(mixBuf, kBlockSize) > 1e-6f)
            receivedAudio = true;
    }

    CHECK(!anyFiniteFailure);
    CHECK(receivedAudio);
}

//==============================================================================
// Test 3 — Multi-route coupling (AmpToFilter + LFOToPitch, 100 blocks)
//==============================================================================

TEST_CASE("Pipeline - multi-route (AmpToFilter + LFOToPitch) no NaN/Inf in 100 blocks", "[pipeline][multi-route]")
{
    constexpr double kSampleRate = 44100.0;
    constexpr int kBlockSize = 512;
    constexpr int kNumBlocks = 100;

    auto snapEngine = std::make_unique<SnapEngine>();
    auto bobEngine = std::make_unique<BobEngine>();

    snapEngine->prepare(kSampleRate, kBlockSize);
    snapEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    snapEngine->reset();
    bobEngine->prepare(kSampleRate, kBlockSize);
    bobEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    bobEngine->reset();

    MegaCouplingMatrix matrix;
    matrix.prepare(kBlockSize, kSampleRate);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs = {snapEngine.get(), bobEngine.get(), nullptr,
                                                                         nullptr, nullptr};
    matrix.setEngines(enginePtrs);

    {
        MegaCouplingMatrix::CouplingRoute r;
        r.sourceSlot = 0;
        r.destSlot = 1;
        r.type = CouplingType::AmpToFilter;
        r.amount = 0.4f;
        r.isNormalled = false;
        r.active = true;
        matrix.addRoute(r);
    }
    {
        MegaCouplingMatrix::CouplingRoute r;
        r.sourceSlot = 0;
        r.destSlot = 1;
        r.type = CouplingType::LFOToPitch;
        r.amount = 0.3f;
        r.isNormalled = false;
        r.active = true;
        matrix.addRoute(r);
    }

    CHECK(matrix.getRoutes().size() == 2);

    juce::AudioBuffer<float> snapBuf(2, kBlockSize);
    juce::AudioBuffer<float> bobBuf(2, kBlockSize);
    juce::AudioBuffer<float> mixBuf(2, kBlockSize);
    bool anyFiniteFailure = false;

    for (int b = 0; b < kNumBlocks && !anyFiniteFailure; ++b)
    {
        snapBuf.clear();
        bobBuf.clear();
        mixBuf.clear();

        juce::MidiBuffer midiBlock;
        if (b == 0)
        {
            midiBlock = makeNoteOn(1, 60, 100);
            snapEngine->wakeSilenceGate();
            bobEngine->wakeSilenceGate();
        }

        snapEngine->renderBlock(snapBuf, midiBlock, kBlockSize);
        auto routes = matrix.loadRoutes();
        matrix.processBlock(kBlockSize, routes);
        juce::MidiBuffer emptyMidi;
        bobEngine->renderBlock(bobBuf, emptyMidi, kBlockSize);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* sd = snapBuf.getReadPointer(ch);
            const float* bd = bobBuf.getReadPointer(ch);
            float* md = mixBuf.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                md[i] = (sd[i] + bd[i]) * 0.5f;
        }

        if (!allFinite(mixBuf, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }
    }

    CHECK(!anyFiniteFailure);
}

//==============================================================================
// Test 4 — Note-on / note-off cycle (SnapEngine)
//==============================================================================

TEST_CASE("Pipeline - note-on/note-off cycle no NaN/Inf across lifecycle", "[pipeline][note-cycle]")
{
    constexpr double kSampleRate = 44100.0;
    constexpr int kBlockSize = 512;

    auto engine = std::make_unique<SnapEngine>();
    engine->prepare(kSampleRate, kBlockSize);
    engine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    engine->reset();

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    bool noteOnPhaseOK = true, noteOffPhaseOK = true;

    for (int b = 0; b < 10; ++b)
    {
        buffer.clear();
        juce::MidiBuffer midi = (b == 0) ? makeNoteOn(1, 60, 100) : juce::MidiBuffer{};
        if (b == 0)
            engine->wakeSilenceGate();
        engine->renderBlock(buffer, midi, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
            noteOnPhaseOK = false;
    }

    {
        buffer.clear();
        juce::MidiBuffer noteOffMidi = makeNoteOff(1, 60);
        engine->renderBlock(buffer, noteOffMidi, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
            noteOffPhaseOK = false;
    }

    for (int b = 0; b < 50; ++b)
    {
        buffer.clear();
        juce::MidiBuffer emptyMidi;
        engine->renderBlock(buffer, emptyMidi, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
        {
            noteOffPhaseOK = false;
            break;
        }
    }

    CHECK(noteOnPhaseOK);
    CHECK(noteOffPhaseOK);
}

//==============================================================================
// Test 5 — KnotTopology bidirectional pipeline stability (100 blocks)
//==============================================================================

TEST_CASE("Pipeline - KnotTopology bidirectional no NaN/Inf in 100 blocks", "[pipeline][knot]")
{
    constexpr double kSampleRate = 44100.0;
    constexpr int kBlockSize = 512;
    constexpr int kNumBlocks = 100;

    auto snapEngine = std::make_unique<SnapEngine>();
    auto bobEngine = std::make_unique<BobEngine>();

    snapEngine->prepare(kSampleRate, kBlockSize);
    snapEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    snapEngine->reset();
    bobEngine->prepare(kSampleRate, kBlockSize);
    bobEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    bobEngine->reset();

    MegaCouplingMatrix matrix;
    matrix.prepare(kBlockSize, kSampleRate);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs = {snapEngine.get(), bobEngine.get(), nullptr,
                                                                         nullptr, nullptr};
    matrix.setEngines(enginePtrs);

    MegaCouplingMatrix::CouplingRoute knotRoute;
    knotRoute.sourceSlot = 0;
    knotRoute.destSlot = 1;
    knotRoute.type = CouplingType::KnotTopology;
    knotRoute.amount = 0.5f;
    knotRoute.isNormalled = false;
    knotRoute.active = true;
    matrix.addRoute(knotRoute);

    CHECK(matrix.getRoutes().size() == 1);

    juce::AudioBuffer<float> snapBuf(2, kBlockSize);
    juce::AudioBuffer<float> bobBuf(2, kBlockSize);
    juce::AudioBuffer<float> mixBuf(2, kBlockSize);
    bool anyFiniteFailure = false;
    bool receivedAudio = false;

    for (int b = 0; b < kNumBlocks && !anyFiniteFailure; ++b)
    {
        snapBuf.clear();
        bobBuf.clear();
        mixBuf.clear();

        juce::MidiBuffer midiBlock;
        if (b == 0)
        {
            midiBlock = makeNoteOn(1, 60, 100);
            snapEngine->wakeSilenceGate();
            bobEngine->wakeSilenceGate();
        }

        juce::MidiBuffer emptyMidi;
        snapEngine->renderBlock(snapBuf, midiBlock, kBlockSize);
        bobEngine->renderBlock(bobBuf, emptyMidi, kBlockSize);

        auto routes = matrix.loadRoutes();
        matrix.processBlock(kBlockSize, routes);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* sd = snapBuf.getReadPointer(ch);
            const float* bd = bobBuf.getReadPointer(ch);
            float* md = mixBuf.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                md[i] = (sd[i] + bd[i]) * 0.5f;
        }

        if (!allFinite(mixBuf, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }
        if (rmsOf(mixBuf, kBlockSize) > 1e-6f)
            receivedAudio = true;
    }

    CHECK(!anyFiniteFailure);
    CHECK(receivedAudio);
}

//==============================================================================
// Test 6 — Engine reset mid-playback (preset-change simulation)
//==============================================================================

TEST_CASE("Pipeline - engine reset mid-playback no NaN/Inf before and after reset", "[pipeline][reset]")
{
    constexpr double kSampleRate = 44100.0;
    constexpr int kBlockSize = 512;

    auto engine = std::make_unique<SnapEngine>();
    engine->prepare(kSampleRate, kBlockSize);
    engine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    engine->reset();

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    bool preResetOK = true, postResetOK = true;

    for (int b = 0; b < 10; ++b)
    {
        buffer.clear();
        juce::MidiBuffer midi = (b == 0) ? makeNoteOn(1, 60, 100) : juce::MidiBuffer{};
        if (b == 0)
            engine->wakeSilenceGate();
        engine->renderBlock(buffer, midi, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
            preResetOK = false;
    }

    engine->reset();
    engine->wakeSilenceGate();

    for (int b = 0; b < 10; ++b)
    {
        buffer.clear();
        juce::MidiBuffer midi = (b == 0) ? makeNoteOn(1, 72, 80) : juce::MidiBuffer{};
        engine->renderBlock(buffer, midi, kBlockSize);
        if (!allFinite(buffer, kBlockSize))
        {
            postResetOK = false;
            break;
        }
    }

    CHECK(preResetOK);
    CHECK(postResetOK);
}

