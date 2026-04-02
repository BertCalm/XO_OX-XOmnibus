// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>

namespace xoceanus {

//==============================================================================
// EngineProfiler — Real-time CPU measurement for audio engine profiling.
//
// Usage in renderBlock():
//     EngineProfiler::ScopedMeasurement m (profiler);
//     // ... render audio ...
//     // measurement is automatically recorded when `m` goes out of scope
//
// Usage from UI thread:
//     auto stats = profiler.getStats();
//     float cpuPercent = stats.avgCpuPercent;
//
// Design:
//   - Zero-allocation (all buffers are pre-sized)
//   - Lock-free (audio thread writes, UI thread reads via atomics)
//   - Ring buffer of last 256 measurements for percentile stats
//   - Configurable CPU budget alarm threshold
//==============================================================================
class EngineProfiler
{
public:
    static constexpr int kHistorySize = 256; // must be power of 2

    struct Stats
    {
        float avgCpuPercent   = 0.0f;   // rolling average
        float peakCpuPercent  = 0.0f;   // peak in current window
        float p95CpuPercent   = 0.0f;   // 95th percentile
        float avgMicroseconds = 0.0f;   // average block time in us
        float peakMicroseconds = 0.0f;  // peak block time in us
        int   blockSize       = 0;
        float sampleRate      = 0.0f;
        bool  budgetExceeded  = false;  // true if any block exceeded budget
    };

    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        sr = sampleRate;
        currentBlockSize = maxBlockSize;

        // Compute the audio budget for one block in microseconds
        if (sampleRate > 0 && maxBlockSize > 0)
            blockBudgetUs = (static_cast<double> (maxBlockSize) / sampleRate) * 1.0e6;
        else
            blockBudgetUs = 1000.0;

        reset();
    }

    void reset() noexcept
    {
        writePos = 0;
        std::memset (historyUs, 0, sizeof (historyUs));
        peakUs.store (0.0f, std::memory_order_relaxed);
        avgUs.store (0.0f, std::memory_order_relaxed);
        budgetExceeded.store (false, std::memory_order_relaxed);
        rollingSum = 0.0;
        rollingCount = 0;
    }

    // Set the CPU budget threshold (as fraction of one block's real-time budget).
    // Default is 1.0 (100%). Set to e.g. 0.22 for Organon's 22% budget.
    void setCpuBudgetFraction (float fraction) noexcept
    {
        budgetFraction = fraction;
    }

    // Record a measurement in microseconds. Called from audio thread.
    void recordBlockTime (float microseconds) noexcept
    {
        // Store in ring buffer
        int pos = writePos & (kHistorySize - 1);
        float old = historyUs[pos];
        historyUs[pos] = microseconds;
        writePos++;

        // Update rolling average
        if (rollingCount < kHistorySize)
        {
            rollingSum += static_cast<double> (microseconds);
            rollingCount++;
        }
        else
        {
            rollingSum += static_cast<double> (microseconds) - static_cast<double> (old);
        }

        float avg = (rollingCount > 0) ? static_cast<float> (rollingSum / rollingCount) : 0.0f;
        avgUs.store (avg, std::memory_order_relaxed);

        // Update peak
        float currentPeak = peakUs.load (std::memory_order_relaxed);
        if (microseconds > currentPeak)
            peakUs.store (microseconds, std::memory_order_relaxed);

        // Check budget
        float budgetUs = static_cast<float> (blockBudgetUs * budgetFraction);
        if (microseconds > budgetUs)
            budgetExceeded.store (true, std::memory_order_relaxed);
    }

    // Get statistics snapshot. Safe to call from UI/message thread.
    Stats getStats() const noexcept
    {
        Stats s;
        s.sampleRate = static_cast<float> (sr);
        s.blockSize = currentBlockSize;
        s.avgMicroseconds = avgUs.load (std::memory_order_relaxed);
        s.peakMicroseconds = peakUs.load (std::memory_order_relaxed);
        s.budgetExceeded = budgetExceeded.load (std::memory_order_relaxed);

        float budgetUsVal = static_cast<float> (blockBudgetUs);
        if (budgetUsVal > 0.0f)
        {
            s.avgCpuPercent = (s.avgMicroseconds / budgetUsVal) * 100.0f;
            s.peakCpuPercent = (s.peakMicroseconds / budgetUsVal) * 100.0f;
        }

        // Compute P95 from history (simple sort of snapshot)
        if (rollingCount >= 10)
        {
            // Copy to scratch array and find P95 via partial sort
            float scratch[kHistorySize];
            int count = (rollingCount < kHistorySize) ? rollingCount : kHistorySize;
            std::memcpy (scratch, historyUs, sizeof (float) * static_cast<size_t> (count));

            // Simple insertion sort (only 256 elements max, no allocation)
            for (int i = 1; i < count; ++i)
            {
                float key = scratch[i];
                int j = i - 1;
                while (j >= 0 && scratch[j] > key)
                {
                    scratch[j + 1] = scratch[j];
                    --j;
                }
                scratch[j + 1] = key;
            }

            int p95Index = static_cast<int> (static_cast<float> (count) * 0.95f);
            if (p95Index >= count) p95Index = count - 1;
            s.p95CpuPercent = (budgetUsVal > 0.0f)
                ? (scratch[p95Index] / budgetUsVal) * 100.0f
                : 0.0f;
        }

        return s;
    }

    // Reset peak and budget alarm. Call periodically from UI thread.
    void resetPeak() noexcept
    {
        peakUs.store (0.0f, std::memory_order_relaxed);
        budgetExceeded.store (false, std::memory_order_relaxed);
    }

    //==========================================================================
    // RAII measurement helper. Measures wall-clock time of the enclosing scope.
    //==========================================================================
    class ScopedMeasurement
    {
    public:
        explicit ScopedMeasurement (EngineProfiler& p) noexcept
            : profiler (p), start (Clock::now()) {}

        ~ScopedMeasurement() noexcept
        {
            auto end = Clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start);
            float us = static_cast<float> (elapsed.count()) * 0.001f;
            profiler.recordBlockTime (us);
        }

        ScopedMeasurement (const ScopedMeasurement&) = delete;
        ScopedMeasurement& operator= (const ScopedMeasurement&) = delete;

    private:
        using Clock = std::chrono::steady_clock;
        EngineProfiler& profiler;
        Clock::time_point start;
    };

private:
    double sr = 44100.0;
    int currentBlockSize = 512;
    double blockBudgetUs = 11609.977; // default: 512 samples at 44.1kHz
    float budgetFraction = 1.0f;

    // Ring buffer (audio thread writes, UI thread reads)
    float historyUs[kHistorySize] {};
    int writePos = 0;
    int rollingCount = 0;
    double rollingSum = 0.0;

    // Atomic stats (written by audio thread, read by UI thread)
    std::atomic<float> peakUs { 0.0f };
    std::atomic<float> avgUs { 0.0f };
    std::atomic<bool> budgetExceeded { false };
};

} // namespace xoceanus
