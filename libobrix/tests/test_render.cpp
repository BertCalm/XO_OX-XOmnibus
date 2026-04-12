// SPDX-License-Identifier: MIT
// Render test — verifies audio output from ObrixSDKAdapter.
#include "obrix/ObrixSDKAdapter.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static bool testSilence()
{
    printf("  Test: silence (no MIDI) ... ");
    obrix::ObrixSDKAdapter adapter;
    adapter.prepare(48000.0, 512);

    float left[512] = {};
    float right[512] = {};

    for (int i = 0; i < 100; ++i)
    {
        std::memset(left, 0, sizeof(left));
        std::memset(right, 0, sizeof(right));
        adapter.renderBlock(left, right, 512, nullptr, 0);
    }

    for (int s = 0; s < 512; ++s)
    {
        if (left[s] != 0.0f || right[s] != 0.0f)
        {
            printf("FAIL — non-zero sample at %d: L=%.6f R=%.6f\n", s, left[s], right[s]);
            return false;
        }
    }
    printf("PASS\n");
    return true;
}

static bool testSingleNote()
{
    printf("  Test: single note (C4) ... ");
    obrix::ObrixSDKAdapter adapter;
    adapter.prepare(48000.0, 512);

    float left[512] = {};
    float right[512] = {};

    // NoteOn: channel 1, note 60, velocity 100
    obrix::MidiEvent noteOn;
    noteOn.sampleOffset = 0;
    noteOn.data[0] = 0x90;
    noteOn.data[1] = 60;
    noteOn.data[2] = 100;
    noteOn.numBytes = 3;

    // First block with note
    adapter.renderBlock(left, right, 512, &noteOn, 1);

    // Subsequent blocks
    float peak = 0.0f;
    for (int block = 0; block < 20; ++block)
    {
        std::memset(left, 0, sizeof(left));
        std::memset(right, 0, sizeof(right));
        adapter.renderBlock(left, right, 512, nullptr, 0);

        for (int s = 0; s < 512; ++s)
        {
            float absL = std::fabs(left[s]);
            float absR = std::fabs(right[s]);
            if (absL > peak) peak = absL;
            if (absR > peak) peak = absR;
        }
    }

    if (peak < 0.001f)
    {
        printf("FAIL — no audio output (peak=%.6f)\n", peak);
        return false;
    }
    if (peak > 4.0f)
    {
        printf("FAIL — peak too high (%.2f)\n", peak);
        return false;
    }

    printf("PASS (peak=%.4f)\n", peak);
    return true;
}

static bool testPolyphony()
{
    printf("  Test: 8-voice polyphony ... ");
    obrix::ObrixSDKAdapter adapter;
    adapter.prepare(48000.0, 512);

    adapter.setParameter("obrix_polyphony", 3.0f); // Poly8

    obrix::MidiEvent notes[8];
    for (int i = 0; i < 8; ++i)
    {
        notes[i].sampleOffset = 0;
        notes[i].data[0] = 0x90;
        notes[i].data[1] = static_cast<uint8_t>(48 + i * 3);
        notes[i].data[2] = 100;
        notes[i].numBytes = 3;
    }

    float left[512] = {};
    float right[512] = {};
    adapter.renderBlock(left, right, 512, notes, 8);

    for (int block = 0; block < 10; ++block)
    {
        std::memset(left, 0, sizeof(left));
        std::memset(right, 0, sizeof(right));
        adapter.renderBlock(left, right, 512, nullptr, 0);
    }

    int voiceCount = adapter.getActiveVoiceCount();
    if (voiceCount < 1)
    {
        printf("FAIL — voiceCount=%d\n", voiceCount);
        return false;
    }

    printf("PASS (activeVoices=%d)\n", voiceCount);
    return true;
}

static bool testParamSweep()
{
    printf("  Test: parameter sweep ... ");
    obrix::ObrixSDKAdapter adapter;
    adapter.prepare(48000.0, 512);

    auto defs = adapter.getParameterList();
    for (const auto& def : defs)
        adapter.setParameter(def.id, def.defaultValue);

    obrix::MidiEvent noteOn;
    noteOn.sampleOffset = 0;
    noteOn.data[0] = 0x90;
    noteOn.data[1] = 60;
    noteOn.data[2] = 127;
    noteOn.numBytes = 3;

    float left[512] = {};
    float right[512] = {};

    for (int block = 0; block < 10; ++block)
    {
        std::memset(left, 0, sizeof(left));
        std::memset(right, 0, sizeof(right));
        adapter.renderBlock(left, right, 512,
                            block == 0 ? &noteOn : nullptr,
                            block == 0 ? 1 : 0);

        for (int s = 0; s < 512; ++s)
        {
            if (!std::isfinite(left[s]) || !std::isfinite(right[s]))
            {
                printf("FAIL — NaN/Inf at block %d sample %d\n", block, s);
                return false;
            }
        }
    }

    printf("PASS (%d params)\n", static_cast<int>(defs.size()));
    return true;
}

int main()
{
    printf("ObrixSDKAdapter render tests\n");
    printf("============================\n");

    int passed = 0, failed = 0;

    if (testSilence()) ++passed; else ++failed;
    if (testSingleNote()) ++passed; else ++failed;
    if (testPolyphony()) ++passed; else ++failed;
    if (testParamSweep()) ++passed; else ++failed;

    printf("\nResults: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
