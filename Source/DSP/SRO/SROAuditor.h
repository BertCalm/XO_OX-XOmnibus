#pragma once
#include "../EngineProfiler.h"
#include <array>
#include <atomic>
#include <cstring>

namespace xolokun {

//==============================================================================
// SROAuditor — Sustainability & Resource Optimization diagnostic.
//
// Sits on top of EngineProfiler to provide the "Earn Your Place" cost-benefit
// analysis. Monitors all 5 engine slots and reports:
//   - Per-slot CPU % and ROI (creative value / performance cost)
//   - Total 5-slot CPU budget utilization
//   - Silence gate status per slot (active / bypassed)
//   - Control-rate vs audio-rate ratio recommendations
//
// Usage:
//     SROAuditor auditor;
//     auditor.prepare(sampleRate, blockSize);
//
//     // After each engine's renderBlock():
//     auditor.recordSlot(slotIndex, profiler.getStats(), silenceGate.isBypassed());
//
//     // From UI thread:
//     auto report = auditor.getReport();
//
// Design:
//   - Zero allocation (fixed arrays)
//   - Lock-free reads via atomic snapshot pattern
//   - Budget alarm at configurable total CPU threshold
//==============================================================================
class SROAuditor
{
public:
    static constexpr int MaxSlots = 5;  // 4 primary + 1 Ghost Slot
    static constexpr float kDefaultBudgetPercent = 70.0f; // alarm at 70% total

    struct SlotReport
    {
        float cpuPercent     = 0.0f;  // average CPU for this slot
        float peakPercent    = 0.0f;  // peak CPU for this slot
        bool  silenceBypassed = false; // true if SilenceGate has bypassed this slot
        bool  active         = false;  // true if an engine is loaded in this slot
        float roi            = 0.0f;  // creative value / CPU cost (higher = better)
    };

    struct Report
    {
        std::array<SlotReport, MaxSlots> slots;
        float totalCpuPercent   = 0.0f;
        float totalPeakPercent  = 0.0f;
        bool  budgetAlarm       = false; // true if total exceeds budget
        int   activeSlots       = 0;
        int   bypassedSlots     = 0;
        float efficiencyScore   = 0.0f; // 0-100, higher = more efficient use of CPU
    };

    void prepare (double /*sampleRate*/, int /*blockSize*/) noexcept
    {
        reset();
    }

    void reset() noexcept
    {
        for (int i = 0; i < MaxSlots; ++i)
        {
            slotCpu[i].store (0.0f, std::memory_order_relaxed);
            slotPeak[i].store (0.0f, std::memory_order_relaxed);
            slotBypassed[i].store (false, std::memory_order_relaxed);
            slotActive[i].store (false, std::memory_order_relaxed);
            slotRoi[i].store (0.0f, std::memory_order_relaxed);
        }
    }

    /// Set the total CPU budget alarm threshold (percent of one block's real-time).
    void setBudgetAlarm (float percent) noexcept { budgetThreshold = percent; }

    //--------------------------------------------------------------------------
    /// Record a slot's stats after rendering. Call from audio thread.
    /// @param slot          Slot index (0–3).
    /// @param stats         EngineProfiler stats for this slot.
    /// @param isBypassed    SilenceGate bypass state.
    /// @param sonicUniqueness  Engine's creative value score (0–1). Higher for
    ///                         engines with unique 6D DNA fingerprints.
    ///
    /// sonicUniqueness MUST be explicitly provided per engine. Derive it from:
    ///   - 6D Sonic DNA variance: distance from fleet centroid in the
    ///     (brightness, warmth, movement, density, space, aggression) space.
    ///     Engines at the extremes (ORACLE: high aggression; OPAL: high space)
    ///     score closer to 1.0. Engines near center score closer to 0.3.
    ///   - OR from seance consensus score normalized to [0, 1]:
    ///     e.g., ORGANON (8/8 → 1.0), OBLIQUE (7.2/10 → 0.72).
    ///   - A default of 0.5 is provided for early integration but should be
    ///     replaced with per-engine values before the ROI metric is trusted.
    void recordSlot (int slot, const EngineProfiler::Stats& stats,
                     bool isBypassed, float sonicUniqueness = 0.5f) noexcept
    {
        if (slot < 0 || slot >= MaxSlots) return;

        slotCpu[slot].store (stats.avgCpuPercent, std::memory_order_relaxed);
        slotPeak[slot].store (stats.peakCpuPercent, std::memory_order_relaxed);
        slotBypassed[slot].store (isBypassed, std::memory_order_relaxed);
        slotActive[slot].store (true, std::memory_order_relaxed);

        // ROI = creative value / CPU cost. Higher is better.
        // If CPU is near zero (bypassed), ROI is infinite — clamp to 100.
        float cpu = stats.avgCpuPercent;
        float roi = (cpu > 0.01f) ? (sonicUniqueness / (cpu * 0.01f)) : 100.0f;
        slotRoi[slot].store (roi, std::memory_order_relaxed);
    }

    /// Mark a slot as inactive (no engine loaded).
    void clearSlot (int slot) noexcept
    {
        if (slot < 0 || slot >= MaxSlots) return;
        slotCpu[slot].store (0.0f, std::memory_order_relaxed);
        slotPeak[slot].store (0.0f, std::memory_order_relaxed);
        slotBypassed[slot].store (false, std::memory_order_relaxed);
        slotActive[slot].store (false, std::memory_order_relaxed);
        slotRoi[slot].store (0.0f, std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    /// Get a full report snapshot. Safe to call from UI/message thread.
    Report getReport() const noexcept
    {
        Report r;
        float totalCpu = 0.0f;
        float totalPeak = 0.0f;

        for (int i = 0; i < MaxSlots; ++i)
        {
            r.slots[i].cpuPercent      = slotCpu[i].load (std::memory_order_relaxed);
            r.slots[i].peakPercent     = slotPeak[i].load (std::memory_order_relaxed);
            r.slots[i].silenceBypassed = slotBypassed[i].load (std::memory_order_relaxed);
            r.slots[i].active          = slotActive[i].load (std::memory_order_relaxed);
            r.slots[i].roi             = slotRoi[i].load (std::memory_order_relaxed);

            if (r.slots[i].active)
            {
                r.activeSlots++;
                if (!r.slots[i].silenceBypassed)
                    totalCpu += r.slots[i].cpuPercent;
                totalPeak += r.slots[i].peakPercent;
                if (r.slots[i].silenceBypassed)
                    r.bypassedSlots++;
            }
        }

        r.totalCpuPercent  = totalCpu;
        r.totalPeakPercent = totalPeak;
        r.budgetAlarm      = totalCpu > budgetThreshold;

        // Efficiency: how well is CPU being used for creative output?
        // 100 = all active slots producing sound; 0 = all slots idle but consuming CPU
        if (r.activeSlots > 0)
        {
            int producing = r.activeSlots - r.bypassedSlots;
            float avgRoi = 0.0f;
            for (int i = 0; i < MaxSlots; ++i)
                if (r.slots[i].active && !r.slots[i].silenceBypassed)
                    avgRoi += r.slots[i].roi;
            if (producing > 0)
                avgRoi /= static_cast<float> (producing);

            // Score combines: producing ratio + ROI quality
            float producingRatio = static_cast<float> (producing) / static_cast<float> (r.activeSlots);
            r.efficiencyScore = producingRatio * 50.0f + (avgRoi > 1.0f ? 50.0f : avgRoi * 50.0f);
        }

        return r;
    }

private:
    float budgetThreshold = kDefaultBudgetPercent;

    std::atomic<float> slotCpu[MaxSlots]  {};
    std::atomic<float> slotPeak[MaxSlots] {};
    std::atomic<bool>  slotBypassed[MaxSlots] {};
    std::atomic<bool>  slotActive[MaxSlots] {};
    std::atomic<float> slotRoi[MaxSlots]  {};
};

} // namespace xolokun
