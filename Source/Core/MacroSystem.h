#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

namespace xomnibus {

//==============================================================================
// MacroTarget — describes a single parameter modulated by a macro knob.
//
// Each macro can drive multiple targets simultaneously with independent ranges.
// The `inverted` flag flips the mapping so macro-at-max yields minValue.
//
struct MacroTarget {
    juce::String engineId;           // Which engine this targets (e.g., "Snap", "Dub")
    juce::String parameterId;        // The APVTS parameter to modulate
    float minValue = 0.0f;           // Macro at 0 → this value
    float maxValue = 1.0f;           // Macro at 1 → this value
    bool inverted = false;           // If true, macro at 1 → minValue
};

//==============================================================================
// MacroSystem — 4 macro knobs that drive engine-specific parameters.
//
// The four macros — CHARACTER (M1), MOVEMENT (M2), COUPLING (M3), SPACE (M4) —
// provide unified performance control across all engines. Each macro reads its
// own APVTS parameter ("macro1" through "macro4") and applies the interpolated
// value to its assigned target parameters.
//
// Processing is audio-thread safe: no allocations, no string lookups per block.
// Macro parameter pointers are cached at construction. Target parameter pointers
// are cached when targets are assigned via setTargets().
//
// SmoothedValue provides ~20ms smoothing to eliminate zipper noise during
// macro sweeps.
//
// Usage:
//   MacroSystem macros(apvts);
//   macros.setTargets(0, { {"Snap", "snap_morphPosition", 0.0f, 1.0f} });
//   // In processBlock:
//   macros.processBlock();
//
class MacroSystem {
public:
    static constexpr int NumMacros = 4;

    // Default labels matching the XOmnibus spec
    static constexpr std::array<const char*, 4> DefaultLabels = {
        "CHARACTER", "MOVEMENT", "COUPLING", "SPACE"
    };

    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    explicit MacroSystem(juce::AudioProcessorValueTreeState& apvts)
        : apvtsRef(apvts)
    {
        // Cache raw pointers to the 4 macro parameters — these are stable for
        // the lifetime of the APVTS and safe to dereference on the audio thread.
        for (int i = 0; i < NumMacros; ++i)
        {
            auto paramId = juce::String("macro") + juce::String(i + 1);
            macroParams[static_cast<size_t>(i)] = apvtsRef.getRawParameterValue(paramId);
            jassert(macroParams[static_cast<size_t>(i)] != nullptr);

            labels[static_cast<size_t>(i)] = DefaultLabels[static_cast<size_t>(i)];

            // Initialize smoothed values — will be reset in prepare()
            smoothedValues[static_cast<size_t>(i)].setCurrentAndTargetValue(0.0f);
        }
    }

    //--------------------------------------------------------------------------
    // Lifecycle
    //--------------------------------------------------------------------------

    // Call from prepareToPlay() to configure smoothing ramp length.
    void prepare(double sampleRate, int maxBlockSize)
    {
        juce::ignoreUnused(maxBlockSize);

        for (int i = 0; i < NumMacros; ++i)
        {
            // ~20ms smoothing to prevent zipper noise
            smoothedValues[static_cast<size_t>(i)].reset(sampleRate, 0.02);

            // Initialize to current parameter value
            auto* param = macroParams[static_cast<size_t>(i)];
            if (param != nullptr)
                smoothedValues[static_cast<size_t>(i)].setCurrentAndTargetValue(param->load());
        }
    }

    //--------------------------------------------------------------------------
    // Target management
    //--------------------------------------------------------------------------

    // Set the targets for a macro. Replaces any existing targets.
    // Caches raw parameter pointers for each target for audio-thread access.
    void setTargets(int macroIndex, std::vector<MacroTarget> targets)
    {
        if (!isValidIndex(macroIndex))
            return;

        auto& slot = macroSlots[static_cast<size_t>(macroIndex)];

        // Build new targets and cache outside the lock to minimise hold time.
        std::vector<std::atomic<float>*> newCached;
        newCached.reserve(targets.size());
        for (const auto& target : targets)
        {
            newCached.push_back(apvtsRef.getRawParameterValue(target.parameterId));
            // rawParam may be nullptr if the target engine isn't loaded —
            // processBlock() handles this gracefully.
        }

        // Hold the lock only while swapping the vectors.
        {
            juce::SpinLock::ScopedLockType lock(slot.lock);
            slot.targets      = std::move(targets);
            slot.cachedParams = std::move(newCached);
        }
    }

    // Remove all targets from a specific macro.
    void clearTargets(int macroIndex)
    {
        if (!isValidIndex(macroIndex))
            return;

        auto& slot = macroSlots[static_cast<size_t>(macroIndex)];
        juce::SpinLock::ScopedLockType lock(slot.lock);
        slot.targets.clear();
        slot.cachedParams.clear();
    }

    // Remove all targets from all macros.
    void clearAllTargets()
    {
        for (int i = 0; i < NumMacros; ++i)
            clearTargets(i);
    }

    // Set a custom label for a macro (per-preset override).
    void setLabel(int macroIndex, const juce::String& label)
    {
        if (isValidIndex(macroIndex))
            labels[static_cast<size_t>(macroIndex)] = label;
    }

    //--------------------------------------------------------------------------
    // Processing — call once per audio block
    //--------------------------------------------------------------------------

    // Read macro knob values and apply them to all assigned targets.
    // Audio-thread safe: no allocations, no string lookups, no blocking.
    //
    // numSamples is needed to advance SmoothedValue correctly per block.
    void processBlock(int numSamples = 512)
    {
        for (int m = 0; m < NumMacros; ++m)
        {
            auto idx = static_cast<size_t>(m);
            auto* param = macroParams[idx];
            if (param == nullptr)
                continue;

            // Update the smoothed value target from the APVTS parameter
            smoothedValues[idx].setTargetValue(param->load());

            // Advance smoother to the end of this block for correct ramp timing.
            // skip(N-1) then getNextValue() = value at end of block.
            if (numSamples > 1)
                smoothedValues[idx].skip(numSamples - 1);
            const float smoothedMacroValue = smoothedValues[idx].getNextValue();

            // Try to acquire the slot lock — if setTargets() is running on the
            // message thread, skip this macro for one block rather than blocking.
            juce::SpinLock::ScopedTryLockType tryLock(macroSlots[idx].lock);
            if (!tryLock.isLocked())
                continue;

            const auto& slot = macroSlots[idx];
            const auto numTargets = slot.targets.size();

            for (size_t t = 0; t < numTargets; ++t)
            {
                auto* targetParam = slot.cachedParams[t];
                if (targetParam == nullptr)
                    continue;

                const auto& target = slot.targets[t];

                // Compute interpolated value in normalized [0,1] range.
                // minValue/maxValue represent normalized parameter range
                // (matching what APVTS stores internally).
                float value;
                if (target.inverted)
                    value = target.maxValue + (target.minValue - target.maxValue) * smoothedMacroValue;
                else
                    value = target.minValue + (target.maxValue - target.minValue) * smoothedMacroValue;

                // Write to the target parameter's atomic float.
                // The min/max values must be in normalized [0,1] range to match
                // APVTS internal storage. Engine adapters are responsible for
                // specifying normalized ranges in their MacroTarget definitions.
                targetParam->store(value);
            }
        }
    }

    //--------------------------------------------------------------------------
    // Query — safe to call from any thread
    //--------------------------------------------------------------------------

    juce::String getLabel(int macroIndex) const
    {
        if (!isValidIndex(macroIndex))
            return {};
        return labels[static_cast<size_t>(macroIndex)];
    }

    float getValue(int macroIndex) const
    {
        if (!isValidIndex(macroIndex))
            return 0.0f;

        auto* param = macroParams[static_cast<size_t>(macroIndex)];
        return param != nullptr ? param->load() : 0.0f;
    }

    const std::vector<MacroTarget>& getTargets(int macroIndex) const
    {
        jassert(isValidIndex(macroIndex));
        return macroSlots[static_cast<size_t>(macroIndex)].targets;
    }

    //--------------------------------------------------------------------------
    // Preset loading
    //--------------------------------------------------------------------------

    // Load macro configuration from preset data.
    //
    // macroLabels: 4-element array of label strings (from "macroLabels" in .xometa)
    // macroTargetsVar: JSON array of target objects (from "macroTargets" in .xometa)
    //
    // Each target object in the JSON array:
    //   { "macro": 1, "engineId": "Snap", "parameterId": "snap_morph",
    //     "min": 0.0, "max": 1.0, "inverted": false }
    //
    void loadFromPreset(const juce::StringArray& macroLabels,
                        const juce::var& macroTargetsVar)
    {
        // Reset everything
        clearAllTargets();

        // Apply labels (fall back to defaults if fewer than 4 provided)
        for (int i = 0; i < NumMacros; ++i)
        {
            if (i < macroLabels.size() && macroLabels[i].isNotEmpty())
                labels[static_cast<size_t>(i)] = macroLabels[i];
            else
                labels[static_cast<size_t>(i)] = DefaultLabels[static_cast<size_t>(i)];
        }

        // Parse target array
        if (!macroTargetsVar.isArray())
            return;

        // Group targets by macro index, then assign in bulk via setTargets
        std::array<std::vector<MacroTarget>, NumMacros> grouped;

        const auto* targetsArray = macroTargetsVar.getArray();
        if (targetsArray == nullptr)
            return;

        for (const auto& entry : *targetsArray)
        {
            int macroIdx = static_cast<int>(entry.getProperty("macro", 0)) - 1; // 1-based in JSON
            if (!isValidIndex(macroIdx))
                continue;

            MacroTarget target;
            target.engineId   = entry.getProperty("engineId", "").toString();
            target.parameterId = entry.getProperty("parameterId", "").toString();
            target.minValue   = static_cast<float>(entry.getProperty("min", 0.0));
            target.maxValue   = static_cast<float>(entry.getProperty("max", 1.0));
            target.inverted   = static_cast<bool>(entry.getProperty("inverted", false));

            if (target.parameterId.isNotEmpty())
                grouped[static_cast<size_t>(macroIdx)].push_back(std::move(target));
        }

        // Assign all grouped targets (this caches the parameter pointers)
        for (int i = 0; i < NumMacros; ++i)
        {
            if (!grouped[static_cast<size_t>(i)].empty())
                setTargets(i, std::move(grouped[static_cast<size_t>(i)]));
        }
    }

private:
    //--------------------------------------------------------------------------
    // Internal state
    //--------------------------------------------------------------------------

    // Per-macro slot holding targets and their cached parameter pointers.
    //
    // SpinLock guards setTargets() (message thread) against concurrent reads in
    // processBlock() (audio thread).  The audio thread uses tryEnter() so it
    // never blocks — it simply skips a single block while a preset is loading.
    struct MacroSlot {
        std::vector<MacroTarget> targets;
        // Parallel array: cachedParams[i] is the raw pointer for targets[i].
        // May contain nullptrs if target parameter doesn't exist in APVTS.
        std::vector<std::atomic<float>*> cachedParams;
        juce::SpinLock lock;
    };

    static bool isValidIndex(int idx) { return idx >= 0 && idx < NumMacros; }

    juce::AudioProcessorValueTreeState& apvtsRef;

    // Cached raw parameter pointers for "macro1" through "macro4"
    std::array<std::atomic<float>*, NumMacros> macroParams = {};

    // Smoothed macro values to eliminate zipper noise (~20ms ramp)
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, NumMacros> smoothedValues;

    // Per-macro label (default: CHARACTER, MOVEMENT, COUPLING, SPACE)
    std::array<juce::String, NumMacros> labels;

    // Per-macro target slots
    std::array<MacroSlot, NumMacros> macroSlots;
};

} // namespace xomnibus
