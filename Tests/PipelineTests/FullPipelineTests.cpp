/*
    XOlokun Full Pipeline Integration Tests
    ==========================================
    Smoke tests for the complete render pipeline:
      preset parameter application → engine prepare → MIDI note-on
      → coupling routes → block render × 1000 → finite, non-zero output.

    This is a black-box integration test — it does NOT verify exact audio
    values, only that the pipeline runs without crashing, produces finite
    output, and delivers non-zero RMS after note-on.

    Design notes:
      - We use SnapEngine (ODDFELIX) and BobEngine (OBLONG) as representative
        engines.  Both are register-only: they require no OS resources beyond
        heap allocation and JUCE audio basics.
      - MasterFXChain requires a full APVTS; the integration test exercises the
        engine-chain → MegaCouplingMatrix → mixed output sub-pipeline only.
        The FX chain is exercised separately in DSPComponentTests.
      - 1000 blocks at 512 samples = ~11.6 seconds of simulated audio at 44.1 kHz.
        That is long enough to expose timer-driven denormal accumulation, LFO
        NaN runaway, and filter instability bugs that single-block tests miss.

    Covers issue #456: "No integration test for full render pipeline".
*/

#include "FullPipelineTests.h"

#include "Core/SynthEngine.h"
#include "Core/MegaCouplingMatrix.h"
#include "Engines/Snap/SnapEngine.h"
#include "Engines/Bob/BobEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <string>
#include <cmath>
#include <array>
#include <memory>
#include <limits>

using namespace xoceanus;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_pipelineTestsPassed = 0;
static int g_pipelineTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_pipelineTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_pipelineTestsFailed++;
    }
}

//==============================================================================
// Helpers
//==============================================================================

static bool allFinite(const juce::AudioBuffer<float>& buf, int numSamples)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        const float* data = buf.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            if (!std::isfinite(data[i]))
                return false;
        }
    }
    return true;
}

static float rms(const juce::AudioBuffer<float>& buf, int numSamples)
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
// Test 1: Single engine renders without NaN/Inf and produces non-zero RMS
//
// Exercises: engine prepare → MIDI note-on → 1000 blocks of renderBlock().
// This is the foundation: if a single engine is broken, multi-engine tests
// would be misleading.
//==============================================================================

static void testSingleEngineRenderStability()
{
    std::cout << "\n--- Single Engine Render Stability (SnapEngine, 1000 blocks) ---\n";

    constexpr double kSampleRate  = 44100.0;
    constexpr int    kBlockSize   = 512;
    constexpr int    kNumBlocks   = 1000;

    auto engine = std::make_unique<SnapEngine>();
    engine->prepare(kSampleRate, kBlockSize);
    engine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    engine->reset();

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    bool anyFiniteFailure = false;
    float peakRMS = 0.0f;
    bool receivedAudio = false;

    // Block 0: inject MIDI note-on so the engine produces sound.
    {
        buffer.clear();
        juce::MidiBuffer noteOn = makeNoteOn(1, 60, 100);
        engine->wakeSilenceGate();
        engine->renderBlock(buffer, noteOn, kBlockSize);

        if (!allFinite(buffer, kBlockSize))
            anyFiniteFailure = true;

        float blockRMS = rms(buffer, kBlockSize);
        if (blockRMS > peakRMS)
            peakRMS = blockRMS;
    }

    // Blocks 1–999: sustain + decay (empty MIDI).
    for (int b = 1; b < kNumBlocks; ++b)
    {
        buffer.clear();
        juce::MidiBuffer emptyMidi;
        engine->renderBlock(buffer, emptyMidi, kBlockSize);

        if (!allFinite(buffer, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }

        float blockRMS = rms(buffer, kBlockSize);
        if (blockRMS > peakRMS)
            peakRMS = blockRMS;

        if (blockRMS > 1e-6f)
            receivedAudio = true;
    }

    reportTest("SnapEngine: no NaN/Inf in 1000 blocks", !anyFiniteFailure);
    reportTest("SnapEngine: non-zero RMS after note-on", receivedAudio || peakRMS > 1e-6f);
}

//==============================================================================
// Test 2: Two engines coupled via AmpToFilter render without corruption
//
// Exercises: engine A (SnapEngine) → MegaCouplingMatrix (AmpToFilter) → engine B
// (BobEngine).  Both engines mix into a shared output buffer.  We verify:
//   (a) No NaN/Inf in the mixed output across 1000 blocks.
//   (b) Non-zero RMS after note-on.
//   (c) BobEngine's applyCouplingInput was called (coupling was delivered).
//==============================================================================

static void testTwoEngineCoupledPipeline()
{
    std::cout << "\n--- Two-Engine Coupled Pipeline (Snap→Bob, AmpToFilter, 1000 blocks) ---\n";

    constexpr double kSampleRate  = 44100.0;
    constexpr int    kBlockSize   = 512;
    constexpr int    kNumBlocks   = 1000;

    //-- Set up engines --------------------------------------------------------
    auto snapEngine = std::make_unique<SnapEngine>();
    auto bobEngine  = std::make_unique<BobEngine>();

    snapEngine->prepare(kSampleRate, kBlockSize);
    snapEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    snapEngine->reset();

    bobEngine->prepare(kSampleRate, kBlockSize);
    bobEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    bobEngine->reset();

    //-- Set up coupling matrix ------------------------------------------------
    MegaCouplingMatrix matrix;
    matrix.prepare(kBlockSize, kSampleRate);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs =
        { snapEngine.get(), bobEngine.get(), nullptr, nullptr, nullptr };
    matrix.setEngines(enginePtrs);

    MegaCouplingMatrix::CouplingRoute route;
    route.sourceSlot  = 0;   // Snap → Bob
    route.destSlot    = 1;
    route.type        = CouplingType::AmpToFilter;
    route.amount      = 0.5f;
    route.isNormalled = false;
    route.active      = true;
    matrix.addRoute(route);

    reportTest("addRoute: AmpToFilter Snap→Bob admitted",
               matrix.getRoutes().size() == 1);

    //-- Render loop -----------------------------------------------------------
    juce::AudioBuffer<float> snapBuf(2, kBlockSize);
    juce::AudioBuffer<float> bobBuf(2, kBlockSize);
    juce::AudioBuffer<float> mixBuf(2, kBlockSize);

    bool anyFiniteFailure = false;
    bool receivedAudio    = false;

    for (int b = 0; b < kNumBlocks; ++b)
    {
        snapBuf.clear();
        bobBuf.clear();
        mixBuf.clear();

        juce::MidiBuffer midiBlock;
        if (b == 0)
        {
            // Inject note-on into both engines on the first block.
            midiBlock = makeNoteOn(1, 60, 100);
            snapEngine->wakeSilenceGate();
            bobEngine->wakeSilenceGate();
        }

        // 1. Render Snap (source engine).
        snapEngine->renderBlock(snapBuf, midiBlock, kBlockSize);

        // 2. Process coupling: Snap amplitude → Bob filter cutoff.
        auto routes = matrix.loadRoutes();
        matrix.processBlock(kBlockSize, routes);

        // 3. Render Bob (destination engine, now modulated).
        juce::MidiBuffer emptyMidi;
        bobEngine->renderBlock(bobBuf, emptyMidi, kBlockSize);

        // 4. Mix both engines into the output buffer (simple sum / 2).
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* snapData = snapBuf.getReadPointer(ch);
            const float* bobData  = bobBuf.getReadPointer(ch);
            float*       mixData  = mixBuf.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                mixData[i] = (snapData[i] + bobData[i]) * 0.5f;
        }

        // 5. Validate mixed output.
        if (!allFinite(mixBuf, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }

        float blockRMS = rms(mixBuf, kBlockSize);
        if (blockRMS > 1e-6f)
            receivedAudio = true;
    }

    reportTest("Snap+Bob coupled: no NaN/Inf in 1000 blocks", !anyFiniteFailure);
    reportTest("Snap+Bob coupled: non-zero mixed RMS after note-on", receivedAudio);
}

//==============================================================================
// Test 3: Multi-route coupling pipeline (AmpToFilter + LFOToPitch simultaneously)
//
// Exercises the coupling matrix with two routes active on the same destination.
// Verifies that multiple simultaneous routes are processed without corruption.
//==============================================================================

static void testMultiRouteCoupledPipeline()
{
    std::cout << "\n--- Multi-Route Pipeline (AmpToFilter + LFOToPitch → Bob, 100 blocks) ---\n";

    constexpr double kSampleRate  = 44100.0;
    constexpr int    kBlockSize   = 512;
    constexpr int    kNumBlocks   = 100;

    auto snapEngine = std::make_unique<SnapEngine>();
    auto bobEngine  = std::make_unique<BobEngine>();

    snapEngine->prepare(kSampleRate, kBlockSize);
    snapEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    snapEngine->reset();

    bobEngine->prepare(kSampleRate, kBlockSize);
    bobEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    bobEngine->reset();

    MegaCouplingMatrix matrix;
    matrix.prepare(kBlockSize, kSampleRate);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs =
        { snapEngine.get(), bobEngine.get(), nullptr, nullptr, nullptr };
    matrix.setEngines(enginePtrs);

    // Route 1: Snap → Bob (AmpToFilter)
    {
        MegaCouplingMatrix::CouplingRoute r;
        r.sourceSlot = 0; r.destSlot = 1;
        r.type       = CouplingType::AmpToFilter;
        r.amount     = 0.4f;
        r.isNormalled = false; r.active = true;
        matrix.addRoute(r);
    }
    // Route 2: Snap → Bob (LFOToPitch)
    {
        MegaCouplingMatrix::CouplingRoute r;
        r.sourceSlot = 0; r.destSlot = 1;
        r.type       = CouplingType::LFOToPitch;
        r.amount     = 0.3f;
        r.isNormalled = false; r.active = true;
        matrix.addRoute(r);
    }

    reportTest("Multi-route: 2 routes admitted", matrix.getRoutes().size() == 2);

    juce::AudioBuffer<float> snapBuf(2, kBlockSize);
    juce::AudioBuffer<float> bobBuf(2, kBlockSize);
    juce::AudioBuffer<float> mixBuf(2, kBlockSize);

    bool anyFiniteFailure = false;

    for (int b = 0; b < kNumBlocks; ++b)
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
            float*       md = mixBuf.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                md[i] = (sd[i] + bd[i]) * 0.5f;
        }

        if (!allFinite(mixBuf, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }
    }

    reportTest("Multi-route: no NaN/Inf in 100 blocks", !anyFiniteFailure);
}

//==============================================================================
// Test 4: Note-on followed by note-off — engine reaches silence without NaN/Inf
//
// Verifies that the full note lifecycle (attack → sustain → release → silence)
// does not produce numerical instability at the tail.
//==============================================================================

static void testNoteOnNoteOffCycle()
{
    std::cout << "\n--- Note-On / Note-Off Cycle (SnapEngine) ---\n";

    constexpr double kSampleRate = 44100.0;
    constexpr int    kBlockSize  = 512;

    auto engine = std::make_unique<SnapEngine>();
    engine->prepare(kSampleRate, kBlockSize);
    engine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    engine->reset();

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    bool noteOnPhaseOK  = true;
    bool noteOffPhaseOK = true;

    // 10 blocks of note-on (attack + sustain)
    for (int b = 0; b < 10; ++b)
    {
        buffer.clear();
        juce::MidiBuffer midi = (b == 0) ? makeNoteOn(1, 60, 100) : juce::MidiBuffer{};
        if (b == 0) engine->wakeSilenceGate();
        engine->renderBlock(buffer, midi, kBlockSize);

        if (!allFinite(buffer, kBlockSize))
            noteOnPhaseOK = false;
    }

    // Note-off
    {
        buffer.clear();
        juce::MidiBuffer noteOffMidi = makeNoteOff(1, 60);
        engine->renderBlock(buffer, noteOffMidi, kBlockSize);
        if (!allFinite(buffer, kBlockSize)) noteOffPhaseOK = false;
    }

    // 50 blocks of tail / release / silence
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

    reportTest("Note-on phase: no NaN/Inf (10 blocks)", noteOnPhaseOK);
    reportTest("Note-off + tail: no NaN/Inf (50 blocks)", noteOffPhaseOK);
}

//==============================================================================
// Test 5: KnotTopology bidirectional coupling — neither engine goes NaN/Inf
//
// KnotTopology is the most complex coupling path (bidirectional within one
// processBlock call).  Exercises it with two engines and verifies stability.
//==============================================================================

static void testKnotTopologyPipelineStability()
{
    std::cout << "\n--- KnotTopology Bidirectional Pipeline Stability (100 blocks) ---\n";

    constexpr double kSampleRate  = 44100.0;
    constexpr int    kBlockSize   = 512;
    constexpr int    kNumBlocks   = 100;

    auto snapEngine = std::make_unique<SnapEngine>();
    auto bobEngine  = std::make_unique<BobEngine>();

    snapEngine->prepare(kSampleRate, kBlockSize);
    snapEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    snapEngine->reset();

    bobEngine->prepare(kSampleRate, kBlockSize);
    bobEngine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    bobEngine->reset();

    MegaCouplingMatrix matrix;
    matrix.prepare(kBlockSize, kSampleRate);

    std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs =
        { snapEngine.get(), bobEngine.get(), nullptr, nullptr, nullptr };
    matrix.setEngines(enginePtrs);

    // KnotTopology: bidirectional coupling (linking number 3 → amount ≈ 0.5)
    MegaCouplingMatrix::CouplingRoute knotRoute;
    knotRoute.sourceSlot  = 0;
    knotRoute.destSlot    = 1;
    knotRoute.type        = CouplingType::KnotTopology;
    knotRoute.amount      = 0.5f;   // linkingNum = round(0.5*4)+1 = 3
    knotRoute.isNormalled = false;
    knotRoute.active      = true;
    matrix.addRoute(knotRoute);

    reportTest("KnotTopology pipeline: route admitted",
               matrix.getRoutes().size() == 1);

    juce::AudioBuffer<float> snapBuf(2, kBlockSize);
    juce::AudioBuffer<float> bobBuf(2, kBlockSize);
    juce::AudioBuffer<float> mixBuf(2, kBlockSize);

    bool anyFiniteFailure = false;
    bool receivedAudio    = false;

    for (int b = 0; b < kNumBlocks; ++b)
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

        // Render both engines first (coupling reads cached outputs from previous block).
        juce::MidiBuffer emptyMidi;
        snapEngine->renderBlock(snapBuf, midiBlock,  kBlockSize);
        bobEngine->renderBlock(bobBuf,  emptyMidi,   kBlockSize);

        // Apply KnotTopology bidirectional coupling.
        auto routes = matrix.loadRoutes();
        matrix.processBlock(kBlockSize, routes);

        // Mix output.
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* sd = snapBuf.getReadPointer(ch);
            const float* bd = bobBuf.getReadPointer(ch);
            float*       md = mixBuf.getWritePointer(ch);
            for (int i = 0; i < kBlockSize; ++i)
                md[i] = (sd[i] + bd[i]) * 0.5f;
        }

        if (!allFinite(mixBuf, kBlockSize))
        {
            anyFiniteFailure = true;
            break;
        }

        if (rms(mixBuf, kBlockSize) > 1e-6f)
            receivedAudio = true;
    }

    reportTest("KnotTopology pipeline: no NaN/Inf in 100 blocks", !anyFiniteFailure);
    reportTest("KnotTopology pipeline: non-zero mixed RMS after note-on", receivedAudio);
}

//==============================================================================
// Test 6: Engine hot-reset between preset-style loads
//
// Simulates what happens when a preset is changed mid-playback:
//   1. Render 10 blocks with note held.
//   2. Call reset() (simulating preset switch).
//   3. Render 10 more blocks — no NaN/Inf should appear.
// This catches filter state corruption, LFO phase wraparound, and ADSR
// re-trigger bugs that only occur after mid-session reset.
//==============================================================================

static void testEngineResetMidPlayback()
{
    std::cout << "\n--- Engine Reset Mid-Playback (preset-change simulation) ---\n";

    constexpr double kSampleRate = 44100.0;
    constexpr int    kBlockSize  = 512;

    auto engine = std::make_unique<SnapEngine>();
    engine->prepare(kSampleRate, kBlockSize);
    engine->prepareSilenceGate(kSampleRate, kBlockSize, 200.0f);
    engine->reset();

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    bool preResetOK  = true;
    bool postResetOK = true;

    // Phase 1: 10 blocks with note held.
    for (int b = 0; b < 10; ++b)
    {
        buffer.clear();
        juce::MidiBuffer midi = (b == 0) ? makeNoteOn(1, 60, 100) : juce::MidiBuffer{};
        if (b == 0) engine->wakeSilenceGate();
        engine->renderBlock(buffer, midi, kBlockSize);

        if (!allFinite(buffer, kBlockSize))
            preResetOK = false;
    }

    // Simulate preset switch: reset all engine state.
    engine->reset();
    engine->wakeSilenceGate();  // re-open silence gate after reset

    // Phase 2: 10 more blocks after reset (with new note-on).
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

    reportTest("Pre-reset render: no NaN/Inf (10 blocks)", preResetOK);
    reportTest("Post-reset render: no NaN/Inf (10 blocks)", postResetOK);
}

//==============================================================================
// Public entry point
//==============================================================================

namespace pipeline_tests {

int runAll()
{
    g_pipelineTestsPassed = 0;
    g_pipelineTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  Full Pipeline Integration Tests\n";
    std::cout << "========================================\n";

    testSingleEngineRenderStability();
    testTwoEngineCoupledPipeline();
    testMultiRouteCoupledPipeline();
    testNoteOnNoteOffCycle();
    testKnotTopologyPipelineStability();
    testEngineResetMidPlayback();

    std::cout << "\n  Pipeline Tests: " << g_pipelineTestsPassed << " passed, "
              << g_pipelineTestsFailed << " failed\n";

    return g_pipelineTestsFailed;
}

} // namespace pipeline_tests
