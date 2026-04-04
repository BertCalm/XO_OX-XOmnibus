// benchmark_organon.cpp — Standalone CPU benchmark for the Organon engine.
//
// Measures renderBlock() performance across different parameter configurations
// and voice counts. Prints results as a table for easy comparison.
//
// Build:
//   g++ -std=c++17 -O2 -I../Source -DJUCE_MODULE_AVAILABLE_juce_audio_processors=0 \
//       benchmark_organon.cpp -o benchmark_organon
//
// This is a mock benchmark that exercises the DSP math without JUCE dependencies.
// For full-pipeline profiling, use the EngineProfiler integrated into the engine.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

// Inline the DSP components we need to benchmark
namespace xoceanus {
    #include "../Source/DSP/FastMath.h"
}

using namespace xoceanus;

//==============================================================================
// Minimal standalone benchmark of the RK4 modal array (the CPU-critical path)
//==============================================================================

static constexpr int kNumModes = 32;
static constexpr int kBlockSize = 512;
static constexpr double kSampleRate = 48000.0; // 48kHz — standard professional rate; avoid hardcoding 44100

struct BenchmarkModalArray
{
    alignas(16) float x[kNumModes] {};
    alignas(16) float v[kNumModes] {};
    alignas(16) float omega[kNumModes] {};
    alignas(16) float weight[kNumModes] {};

    void setFundamental (float freqHz, float isotopeBalance)
    {
        constexpr float twoPi = 6.28318530717958647692f;
        for (int n = 0; n < kNumModes; ++n)
        {
            float harmonic = static_cast<float> (n + 1);
            float spread = 0.5f + isotopeBalance * 1.5f;
            float modeFreq = freqHz * std::pow (harmonic, spread);
            float nyquist = static_cast<float> (kSampleRate) * 0.49f;
            if (modeFreq > nyquist) modeFreq = nyquist;
            omega[n] = twoPi * modeFreq;
        }
    }

    void setWeights (float centroid, float energy, float catalyst, float entropy, float isotope)
    {
        float bandwidth = 0.15f + 0.35f * isotope;
        float invBw2 = 1.0f / (bandwidth * bandwidth + 0.0001f);
        for (int n = 0; n < kNumModes; ++n)
        {
            float modePos = static_cast<float> (n) / static_cast<float> (kNumModes - 1);
            float dist = modePos - centroid;
            float gaussian = fastExp (-0.5f * dist * dist * invBw2);
            weight[n] = catalyst * energy * entropy * gaussian;
        }
    }

    float processSample (float damping)
    {
        float dt = static_cast<float> (1.0 / kSampleRate);
        float halfDt = dt * 0.5f;
        float output = 0.0f;

        for (int n = 0; n < kNumModes; ++n)
        {
            float w2 = omega[n] * omega[n];
            float F = weight[n];
            float g = damping;

            float x0 = x[n], v0 = v[n];

            float dx1 = v0;
            float dv1 = -w2 * x0 - g * v0 + F;

            float x1 = x0 + halfDt * dx1;
            float v1 = v0 + halfDt * dv1;
            float dx2 = v1;
            float dv2 = -w2 * x1 - g * v1 + F;

            float x2 = x0 + halfDt * dx2;
            float v2 = v0 + halfDt * dv2;
            float dx3 = v2;
            float dv3 = -w2 * x2 - g * v2 + F;

            float x3 = x0 + dt * dx3;
            float v3 = v0 + dt * dv3;
            float dx4 = v3;
            float dv4 = -w2 * x3 - g * v3 + F;

            x[n] = flushDenormal (x0 + (dt / 6.0f) * (dx1 + 2.0f * dx2 + 2.0f * dx3 + dx4));
            v[n] = flushDenormal (v0 + (dt / 6.0f) * (dv1 + 2.0f * dv2 + 2.0f * dv3 + dv4));

            output += x[n];
        }
        return output;
    }
};

struct BenchConfig
{
    const char* name;
    int voiceCount;
    float freqHz;
    float isotope;
    float catalyst;
    float damping;
};

int main()
{
    BenchConfig configs[] = {
        { "1 voice, A4, default",      1, 440.0f, 0.5f, 0.5f, 6.0f },
        { "4 voices, A4, default",     4, 440.0f, 0.5f, 0.5f, 6.0f },
        { "4 voices, C2, sub-heavy",   4, 65.4f,  0.1f, 0.8f, 4.0f },
        { "4 voices, C6, bright",      4, 1046.5f, 0.9f, 1.5f, 8.0f },
        { "4 voices, A4, max catalyst", 4, 440.0f, 0.5f, 2.0f, 6.0f },
    };

    int numConfigs = sizeof (configs) / sizeof (configs[0]);
    int numIterations = 1000; // 1000 blocks per config
    double blockDurationUs = (static_cast<double> (kBlockSize) / kSampleRate) * 1.0e6;

    printf ("XOrganon RK4 Modal Array Benchmark\n");
    printf ("===================================\n");
    printf ("Sample rate: %.0f Hz | Block size: %d | Budget per block: %.1f us\n\n",
            kSampleRate, kBlockSize, blockDurationUs);
    printf ("%-30s | Voices | Avg (us) | Peak (us) | CPU %%   | Status\n", "Configuration");
    printf ("-------------------------------+--------+----------+-----------+---------+--------\n");

    for (int c = 0; c < numConfigs; ++c)
    {
        auto& cfg = configs[c];
        BenchmarkModalArray voices[4];
        float peakUs = 0.0f;
        double totalUs = 0.0;

        // Setup voices
        for (int v = 0; v < cfg.voiceCount; ++v)
        {
            voices[v].setFundamental (cfg.freqHz, cfg.isotope);
            voices[v].setWeights (0.5f, 0.6f, cfg.catalyst, 0.7f, cfg.isotope);
        }

        // Warmup
        for (int iter = 0; iter < 100; ++iter)
        {
            for (int v = 0; v < cfg.voiceCount; ++v)
                for (int s = 0; s < kBlockSize; ++s)
                    voices[v].processSample (cfg.damping);
        }

        // Benchmark
        for (int iter = 0; iter < numIterations; ++iter)
        {
            auto start = std::chrono::steady_clock::now();

            for (int v = 0; v < cfg.voiceCount; ++v)
                for (int s = 0; s < kBlockSize; ++s)
                    voices[v].processSample (cfg.damping);

            auto end = std::chrono::steady_clock::now();
            float elapsed = static_cast<float> (
                std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count()) * 0.001f;

            totalUs += elapsed;
            if (elapsed > peakUs) peakUs = elapsed;
        }

        float avgUs = static_cast<float> (totalUs / numIterations);
        float cpuPercent = (avgUs / static_cast<float> (blockDurationUs)) * 100.0f;
        const char* status = (cpuPercent < 22.0f) ? "OK" : "OVER";

        printf ("%-30s | %6d | %8.1f | %9.1f | %5.1f %% | %s\n",
                cfg.name, cfg.voiceCount, avgUs, peakUs, cpuPercent, status);
    }

    printf ("\nBudget threshold: 22%% of one block's real-time budget.\n");
    return 0;
}
