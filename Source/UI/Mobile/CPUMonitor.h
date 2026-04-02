// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <functional>

namespace xoceanus {

//==============================================================================
// CPUMonitor — Real-time audio thread CPU tracking and quality mitigation.
//
// Measures the ratio of time spent in the audio callback vs. the available
// buffer period. When load exceeds thresholds, triggers graduated mitigation
// to prevent audio glitches.
//
// Degradation order (least audible impact first):
//   1. Reduce UI animation frame rate (60fps → 30fps)
//   2. Halve mod matrix update rate (every 2 blocks)
//   3. Disable oversampling (2x → 1x)
//   4. Simplify FX (shorter reverb, linear interpolation)
//   5. Reduce voice count (oldest-note steal with 5ms fade)
//   * Never reduce sample rate or bit depth
//
// Thread safety:
//   - measureBlock() called on the audio thread (writes atomics)
//   - getLoad() / getQualityLevel() read from any thread
//   - Callback for mitigation level changes fires on message thread
//
class CPUMonitor {
public:
    // Quality levels from full to most degraded
    enum class QualityLevel {
        Full,           // Normal operation
        ReducedUI,      // UI animations throttled
        ReducedMod,     // Mod matrix update rate halved
        Eco,            // Oversampling off, FX simplified
        VoiceReduced    // Voice count reduced
    };

    struct Config {
        float warningThreshold  = 0.70f;   // Show UI badge
        float reducedUIAt       = 0.60f;   // Throttle animations
        float reducedModAt      = 0.70f;   // Halve mod rate
        float ecoAt             = 0.80f;   // Eco quality
        float voiceReducedAt    = 0.90f;   // Voice reduction
        float hysteresis        = 0.05f;   // Must drop this far below threshold to recover
        float smoothingSeconds  = 1.0f;    // EMA time constant for load measurement
    };

    void setConfig(const Config& cfg) { config = cfg; }

    //-- Audio thread interface --------------------------------------------------

    // Call at the START of the audio callback.
    void beginBlock()
    {
        blockStartTicks = juce::Time::getHighResolutionTicks();
    }

    // Call at the END of the audio callback.
    // bufferSize and sampleRate are needed to compute the available time budget.
    void endBlock(int bufferSize, double sampleRate)
    {
        const int64_t endTicks = juce::Time::getHighResolutionTicks();
        const double elapsedSeconds = juce::Time::highResolutionTicksToSeconds(
            endTicks - blockStartTicks);

        const double budgetSeconds = static_cast<double>(bufferSize) / sampleRate;
        const float instantLoad = static_cast<float>(elapsedSeconds / budgetSeconds);

        // EMA smoothing
        float alpha = 1.0f;
        if (config.smoothingSeconds > 0.0f && sampleRate > 0.0)
        {
            float blockDuration = static_cast<float>(budgetSeconds);
            alpha = 1.0f - std::exp(-blockDuration / config.smoothingSeconds);
        }

        float prev = smoothedLoad.load(std::memory_order_relaxed);
        float next = prev + alpha * (instantLoad - prev);
        smoothedLoad.store(next, std::memory_order_relaxed);

        // Track peak (reset periodically by message thread)
        float currentPeak = peakLoad.load(std::memory_order_relaxed);
        if (instantLoad > currentPeak)
            peakLoad.store(instantLoad, std::memory_order_relaxed);

        // Determine quality level (audio thread side — just set atomic)
        QualityLevel level = QualityLevel::Full;
        if (next >= config.voiceReducedAt)
            level = QualityLevel::VoiceReduced;
        else if (next >= config.ecoAt)
            level = QualityLevel::Eco;
        else if (next >= config.reducedModAt)
            level = QualityLevel::ReducedMod;
        else if (next >= config.reducedUIAt)
            level = QualityLevel::ReducedUI;

        // Apply hysteresis: only step DOWN a quality level if load drops
        // below threshold minus hysteresis margin
        QualityLevel current = qualityLevel.load(std::memory_order_relaxed);
        if (level > current)
        {
            // Worsening — respond immediately
            qualityLevel.store(level, std::memory_order_relaxed);
        }
        else if (level < current)
        {
            // Improving — require hysteresis margin
            float currentThreshold = thresholdForLevel(current);
            if (next < currentThreshold - config.hysteresis)
                qualityLevel.store(level, std::memory_order_relaxed);
        }
    }

    //-- Read from any thread ----------------------------------------------------

    float getLoad() const { return smoothedLoad.load(std::memory_order_relaxed); }
    float getPeakLoad() const { return peakLoad.load(std::memory_order_relaxed); }
    void resetPeak() { peakLoad.store(0.0f, std::memory_order_relaxed); }

    QualityLevel getQualityLevel() const
    {
        return qualityLevel.load(std::memory_order_relaxed);
    }

    bool isWarning() const { return getLoad() >= config.warningThreshold; }

    //-- Query helpers for engine code -------------------------------------------

    // Should the mod matrix run at half rate this block?
    bool shouldHalveModRate() const
    {
        auto level = getQualityLevel();
        return level >= QualityLevel::ReducedMod;
    }

    // Should oversampling be disabled?
    bool shouldDisableOversampling() const
    {
        auto level = getQualityLevel();
        return level >= QualityLevel::Eco;
    }

    // Should voice count be reduced?
    bool shouldReduceVoices() const
    {
        auto level = getQualityLevel();
        return level >= QualityLevel::VoiceReduced;
    }

    // Voice count multiplier based on current quality level and engine count
    float voiceMultiplier(int activeEngineCount) const
    {
        // Base multiplier from engine count (per mobile spec)
        float engineFactor = 1.0f;
        if (activeEngineCount == 2) engineFactor = 0.6f;
        else if (activeEngineCount >= 3) engineFactor = 0.4f;

        // Further reduce if CPU is critical
        if (shouldReduceVoices())
            engineFactor *= 0.5f;

        return engineFactor;
    }

    //-- Platform budget ---------------------------------------------------------

    struct PlatformBudget {
        int maxEngines;
        float maxCPU;
        bool ecoAvailable;
    };

    // Returns the CPU budget for the current device class.
    // deviceClass: 0 = iPhone, 1 = iPad, 2 = iPad Pro (M-series)
    static PlatformBudget getBudget(int deviceClass, int activeEngines)
    {
        if (deviceClass == 0) // iPhone
        {
            if (activeEngines == 1)
                return { 2, 0.25f, true };
            return { 2, 0.45f, true };
        }
        else if (deviceClass == 1) // iPad
        {
            if (activeEngines <= 2)
                return { 3, 0.35f, true };
            return { 3, 0.50f, true };
        }
        else // iPad Pro
        {
            if (activeEngines <= 2)
                return { 3, 0.35f, false };
            return { 3, 0.50f, false };
        }
    }

private:
    Config config;
    int64_t blockStartTicks = 0;

    std::atomic<float> smoothedLoad { 0.0f };
    std::atomic<float> peakLoad { 0.0f };
    std::atomic<QualityLevel> qualityLevel { QualityLevel::Full };

    float thresholdForLevel(QualityLevel level) const
    {
        switch (level)
        {
            case QualityLevel::Full:          return 0.0f;
            case QualityLevel::ReducedUI:     return config.reducedUIAt;
            case QualityLevel::ReducedMod:    return config.reducedModAt;
            case QualityLevel::Eco:           return config.ecoAt;
            case QualityLevel::VoiceReduced:  return config.voiceReducedAt;
        }
        return 0.0f;
    }
};

} // namespace xoceanus
