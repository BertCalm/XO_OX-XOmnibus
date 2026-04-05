// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <mutex>
#include <vector>

namespace xoceanus
{

//==============================================================================
// MacroTarget — describes a single parameter modulated by a macro knob.
//
// Each macro can drive multiple targets simultaneously with independent ranges.
// The `inverted` flag flips the mapping so macro-at-max yields minValue.
//
// Coupling route targets:
// When `isCouplingTarget` is true, the macro writes to a coupling route
// amount parameter (`cp_rN_amount`) instead of a regular engine parameter.
// The `parameterId` field is ignored for coupling targets — the target
// parameter is resolved from `couplingRouteSlot` at cache time.
// See coupling_performance_spec.md Section 4 for the design rationale.
//
struct MacroTarget
{
    juce::String engineId;    // Which engine this targets (e.g., "OddfeliX", "Overdub")
    juce::String parameterId; // The APVTS parameter to modulate
    float minValue = 0.0f;    // Macro at 0 → this value
    float maxValue = 1.0f;    // Macro at 1 → this value
    bool inverted = false;    // If true, macro at 1 → minValue

    // Coupling route targeting (see coupling_performance_spec.md §4.1)
    bool isCouplingTarget = false; // True → writes to cp_rN_amount instead of parameterId
    int couplingRouteSlot = -1;    // 0-3, maps to cp_r{slot+1}_amount
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
// Synchronization — lock-free double-buffer per slot:
//   Each MacroSlot holds two TargetBuffer copies indexed by activeBuffer_ (0 or 1).
//   Message thread writes to the INACTIVE buffer, then atomically flips activeBuffer_.
//   Audio thread always reads from activeBuffer_ with memory_order_acquire — no
//   spinning, no try-lock, no dropped updates.
//   Labels are UI-only and protected by a std::mutex (never touched on audio thread).
//
// SmoothedValue provides ~20ms smoothing to eliminate zipper noise during
// macro sweeps.
//
// Coupling route amount targets:
// Macros can also drive coupling route amounts (cp_r1_amount through
// cp_r4_amount) for real-time coupling depth control. Use
// addCouplingTarget() to wire a macro to a coupling route slot.
// macro3 ("COUPLING") is the natural home for coupling depth control,
// mapping to the MPCe quad-corner right axis (see spec §4.2).
//
// Usage:
//   MacroSystem macros(apvts);
//   macros.setTargets(0, { {"OddfeliX", "snap_morphPosition", 0.0f, 1.0f} });
//   macros.addCouplingTarget(2, 0, 0.0f, 1.0f);  // macro3 → route 1 depth
//   // In processBlock:
//   macros.processBlock();
//
class MacroSystem
{
public:
    static constexpr int NumMacros = 4;
    static constexpr int NumCouplingRoutes = 4;

    // Macro index for the COUPLING knob — natural home for coupling depth.
    // Maps to MPCe quad-corner right axis (see coupling_performance_spec.md §4.2).
    static constexpr int CouplingMacroIndex = 2; // macro3 (0-based)

    // Default labels matching the XOceanus spec
    static constexpr std::array<const char*, 4> DefaultLabels = {"CHARACTER", "MOVEMENT", "COUPLING", "SPACE"};

    //--------------------------------------------------------------------------
    // Construction
    //--------------------------------------------------------------------------

    explicit MacroSystem(juce::AudioProcessorValueTreeState& apvts) : apvtsRef(apvts)
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

        // Cache raw pointers to coupling route amount parameters.
        // These may be nullptr if the coupling performance params haven't been
        // added to the APVTS yet (Phase A dependency). processBlock() checks
        // for nullptr before writing.
        for (int i = 0; i < NumCouplingRoutes; ++i)
        {
            auto paramId = juce::String("cp_r") + juce::String(i + 1) + "_amount";
            couplingAmountParams[static_cast<size_t>(i)] = apvtsRef.getRawParameterValue(paramId);
            // Note: may be nullptr — coupling APVTS params are added in Phase A
            // of the coupling performance system. This is intentionally not asserted.
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
    //
    // Message thread only. Writes to the INACTIVE buffer, then flips activeBuffer_
    // atomically — the audio thread sees the new targets on the next processBlock()
    // call without any locking or contention.
    void setTargets(int macroIndex, std::vector<MacroTarget> targets)
    {
        if (!isValidIndex(macroIndex))
            return;

        auto& slot = macroSlots[static_cast<size_t>(macroIndex)];

        // Resolve parameter pointers before touching shared state.
        std::vector<std::atomic<float>*> newCached;
        newCached.reserve(targets.size());
        for (const auto& target : targets)
        {
            if (target.isCouplingTarget && isValidCouplingSlot(target.couplingRouteSlot))
            {
                // Coupling targets use the pre-cached coupling amount pointer
                // instead of looking up parameterId in the APVTS.
                newCached.push_back(couplingAmountParams[static_cast<size_t>(target.couplingRouteSlot)]);
            }
            else
            {
                newCached.push_back(apvtsRef.getRawParameterValue(target.parameterId));
            }
            // rawParam may be nullptr if the target engine isn't loaded or
            // coupling params aren't registered yet — processBlock() handles
            // this gracefully.
        }

        // Write to the INACTIVE buffer (the one the audio thread is NOT reading).
        const int inactive = 1 - slot.activeBuffer.load(std::memory_order_relaxed);
        slot.buffers[static_cast<size_t>(inactive)].targets    = std::move(targets);
        slot.buffers[static_cast<size_t>(inactive)].cachedParams = std::move(newCached);

        // Atomically expose the new buffer to the audio thread.
        slot.activeBuffer.store(inactive, std::memory_order_release);
        slot.dirty.store(true, std::memory_order_release);
    }

    // Remove all targets from a specific macro.
    // Message thread only. Clears both buffers so neither can produce stale writes,
    // then flips to the just-cleared inactive buffer.
    void clearTargets(int macroIndex)
    {
        if (!isValidIndex(macroIndex))
            return;

        auto& slot = macroSlots[static_cast<size_t>(macroIndex)];

        // Clear the inactive buffer and flip, then clear the now-inactive (old active) buffer.
        // After both clears the audio thread will see an empty target list.
        const int inactive = 1 - slot.activeBuffer.load(std::memory_order_relaxed);
        slot.buffers[static_cast<size_t>(inactive)].targets.clear();
        slot.buffers[static_cast<size_t>(inactive)].cachedParams.clear();
        slot.activeBuffer.store(inactive, std::memory_order_release);
        slot.dirty.store(true, std::memory_order_release);

        // Clear the old active buffer (now inactive) so it doesn't hold stale data.
        const int oldActive = 1 - inactive;
        slot.buffers[static_cast<size_t>(oldActive)].targets.clear();
        slot.buffers[static_cast<size_t>(oldActive)].cachedParams.clear();
    }

    // Remove all targets from all macros.
    void clearAllTargets()
    {
        for (int i = 0; i < NumMacros; ++i)
            clearTargets(i);
    }

    // Set a custom label for a macro (per-preset override). Thread-safe (#682).
    // Labels are UI-only — never accessed on the audio thread — so a std::mutex
    // is sufficient and simpler than a double-buffer.
    void setLabel(int macroIndex, const juce::String& label)
    {
        if (isValidIndex(macroIndex))
        {
            const std::lock_guard<std::mutex> lock(labelMutex_);
            labels[static_cast<size_t>(macroIndex)] = label;
        }
    }

    //--------------------------------------------------------------------------
    // Coupling route targeting
    //--------------------------------------------------------------------------

    // Add a coupling route amount as a target for a macro knob.
    //
    // This creates a MacroTarget that writes to the coupling route's amount
    // parameter (cp_rN_amount) instead of an engine parameter. The coupling
    // amount is bipolar (-1.0 to 1.0), so min/max should be set accordingly.
    //
    // macroIndex: 0-3 (which macro knob drives the coupling depth)
    // routeSlot:  0-3 (which coupling performance route to control)
    // min/max:    the range to interpolate within (e.g., 0.0 to 1.0 for
    //             unipolar sweep, or -1.0 to 1.0 for full bipolar sweep)
    //
    // macro3 ("COUPLING", index 2) is the natural home for coupling depth.
    // See coupling_performance_spec.md §4 for design rationale.
    //
    // Example: sweep route 1 depth from 0 to full as macro3 rises:
    //   addCouplingTarget(2, 0, 0.0f, 1.0f);
    //
    void addCouplingTarget(int macroIndex, int routeSlot, float min, float max)
    {
        if (!isValidIndex(macroIndex) || !isValidCouplingSlot(routeSlot))
            return;

        // Build the coupling MacroTarget — engineId and parameterId are left
        // empty since the target is resolved from couplingRouteSlot.
        MacroTarget couplingTarget;
        couplingTarget.isCouplingTarget = true;
        couplingTarget.couplingRouteSlot = routeSlot;
        couplingTarget.minValue = min;
        couplingTarget.maxValue = max;

        // Append to existing targets for this macro (don't replace them).
        // Read the current active buffer's contents, append, then write to the
        // inactive buffer and flip — same double-buffer protocol as setTargets().
        auto& slot = macroSlots[static_cast<size_t>(macroIndex)];

        // Cache the coupling amount parameter pointer.
        auto* cachedParam = couplingAmountParams[static_cast<size_t>(routeSlot)];

        const int active   = slot.activeBuffer.load(std::memory_order_relaxed);
        const int inactive = 1 - active;

        // Copy the live data into the inactive buffer, then append.
        slot.buffers[static_cast<size_t>(inactive)] = slot.buffers[static_cast<size_t>(active)];
        slot.buffers[static_cast<size_t>(inactive)].targets.push_back(std::move(couplingTarget));
        slot.buffers[static_cast<size_t>(inactive)].cachedParams.push_back(cachedParam);

        slot.activeBuffer.store(inactive, std::memory_order_release);
        slot.dirty.store(true, std::memory_order_release);
    }

    //--------------------------------------------------------------------------
    // Processing — call once per audio block
    //--------------------------------------------------------------------------

    // Read macro knob values and apply them to all assigned targets.
    // Audio-thread safe: no allocations, no string lookups, no blocking.
    // Reads from activeBuffer_ with memory_order_acquire — guaranteed to see
    // any setTargets() flip that completed before this call.
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
            smoothedValues[idx].setTargetValue(param->load(std::memory_order_relaxed));

            // Advance smoother to the end of this block for correct ramp timing.
            // skip(N-1) then getNextValue() = value at end of block.
            if (numSamples > 1)
                smoothedValues[idx].skip(numSamples - 1);
            const float smoothedMacroValue = smoothedValues[idx].getNextValue();

            // Lock-free read: load the active buffer index with acquire semantics
            // so all writes made before the store(release) in setTargets() are visible.
            auto& slot = macroSlots[idx];
            const int active = slot.activeBuffer.load(std::memory_order_acquire);
            // Consume and clear the dirty flag (informational — no action required here,
            // but callers monitoring for target changes can inspect it via isDirty()).
            slot.dirty.store(false, std::memory_order_relaxed);

            const auto& buf = slot.buffers[static_cast<size_t>(active)];
            const auto numTargets = buf.targets.size();

            for (size_t t = 0; t < numTargets; ++t)
            {
                auto* targetParam = buf.cachedParams[t];
                if (targetParam == nullptr)
                    continue;

                const auto& target = buf.targets[t];

                // Compute interpolated value between minValue and maxValue.
                // For engine targets: min/max are normalized [0,1] matching
                // APVTS internal storage.
                // For coupling targets: min/max are in [-1,1] range matching
                // the bipolar cp_rN_amount parameter range.
                float value;
                if (target.inverted)
                    value = target.maxValue + (target.minValue - target.maxValue) * smoothedMacroValue;
                else
                    value = target.minValue + (target.maxValue - target.minValue) * smoothedMacroValue;

                // Write to the target parameter's atomic float.
                // Engine adapters are responsible for specifying correct ranges
                // in their MacroTarget definitions. Coupling targets use the
                // pre-cached cp_rN_amount pointer resolved in setTargets().
                targetParam->store(value);
            }
        }
    }

    //--------------------------------------------------------------------------
    // Query — safe to call from any thread
    //--------------------------------------------------------------------------

    juce::String getLabel(int macroIndex) const // Thread-safe (#682) — message thread only
    {
        if (!isValidIndex(macroIndex))
            return {};
        const std::lock_guard<std::mutex> lock(labelMutex_);
        return labels[static_cast<size_t>(macroIndex)];
    }

    float getValue(int macroIndex) const
    {
        if (!isValidIndex(macroIndex))
            return 0.0f;

        auto* param = macroParams[static_cast<size_t>(macroIndex)];
        return param != nullptr ? param->load() : 0.0f;
    }

    // Returns a const reference to the currently active target list.
    // Message thread only — do not call from the audio thread.
    const std::vector<MacroTarget>& getTargets(int macroIndex) const
    {
        jassert(isValidIndex(macroIndex));
        if (!isValidIndex(macroIndex))
        {
            static const std::vector<MacroTarget> empty;
            return empty;
        }
        const auto& slot = macroSlots[static_cast<size_t>(macroIndex)];
        const int active = slot.activeBuffer.load(std::memory_order_acquire);
        return slot.buffers[static_cast<size_t>(active)].targets;
    }

    //--------------------------------------------------------------------------
    // Session state serialization — closes #313
    //--------------------------------------------------------------------------

    // Serialize current macro targets and labels to an XmlElement named "MacroTargets".
    // Call from getStateInformation() to persist live macro wiring across DAW sessions.
    //
    // Format:
    //   <MacroTargets>
    //     <Macro index="0" label="CHARACTER">
    //       <Target engineId="OddfeliX" parameterId="snap_morph" min="0" max="1"
    //               inverted="0" coupling="0" routeSlot="-1"/>
    //       ...
    //     </Macro>
    //     ...
    //   </MacroTargets>
    //
    std::unique_ptr<juce::XmlElement> getState() const
    {
        auto root = std::make_unique<juce::XmlElement>("MacroTargets");

        for (int m = 0; m < NumMacros; ++m)
        {
            auto* macroElem = root->createNewChildElement("Macro");
            macroElem->setAttribute("index", m);
            {
                const std::lock_guard<std::mutex> labelLk(labelMutex_);
                macroElem->setAttribute("label", labels[static_cast<size_t>(m)]);
            }

            // Read from the active buffer — no lock needed (message thread only,
            // no concurrent writer at this point inside getStateInformation()).
            const auto& slot = macroSlots[static_cast<size_t>(m)];
            const int active = slot.activeBuffer.load(std::memory_order_acquire);

            for (const auto& t : slot.buffers[static_cast<size_t>(active)].targets)
            {
                auto* te = macroElem->createNewChildElement("Target");
                te->setAttribute("engineId", t.engineId);
                te->setAttribute("parameterId", t.parameterId);
                te->setAttribute("min", static_cast<double>(t.minValue));
                te->setAttribute("max", static_cast<double>(t.maxValue));
                te->setAttribute("inverted", t.inverted ? 1 : 0);
                te->setAttribute("coupling", t.isCouplingTarget ? 1 : 0);
                te->setAttribute("routeSlot", t.couplingRouteSlot);
            }
        }

        return root;
    }

    // Restore macro targets and labels from an XmlElement previously produced by
    // getState(). Idempotent: missing/malformed data is silently skipped.
    // Call from setStateInformation() after engines are loaded so that
    // parameter pointers can be resolved in setTargets().
    void setState(const juce::XmlElement* root)
    {
        if (root == nullptr || root->getTagName() != "MacroTargets")
            return;

        clearAllTargets();

        for (auto* macroElem : root->getChildIterator())
        {
            if (macroElem->getTagName() != "Macro")
                continue;

            const int m = macroElem->getIntAttribute("index", -1);
            if (!isValidIndex(m))
                continue;

            // Restore label
            juce::String lbl = macroElem->getStringAttribute("label");
            if (lbl.isNotEmpty())
            {
                const std::lock_guard<std::mutex> lock(labelMutex_);
                labels[static_cast<size_t>(m)] = lbl;
            }

            // Collect targets
            std::vector<MacroTarget> targets;
            for (auto* te : macroElem->getChildIterator())
            {
                if (te->getTagName() != "Target")
                    continue;

                MacroTarget t;
                t.engineId = te->getStringAttribute("engineId");
                t.parameterId = te->getStringAttribute("parameterId");
                t.minValue = static_cast<float>(te->getDoubleAttribute("min", 0.0));
                t.maxValue = static_cast<float>(te->getDoubleAttribute("max", 1.0));
                t.inverted = te->getIntAttribute("inverted", 0) != 0;
                t.isCouplingTarget = te->getIntAttribute("coupling", 0) != 0;
                t.couplingRouteSlot = te->getIntAttribute("routeSlot", -1);

                // Basic validation before accepting
                if (t.isCouplingTarget)
                {
                    if (isValidCouplingSlot(t.couplingRouteSlot))
                        targets.push_back(std::move(t));
                }
                else if (t.parameterId.isNotEmpty())
                {
                    targets.push_back(std::move(t));
                }
            }

            if (!targets.empty())
                setTargets(m, std::move(targets));
        }
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
    //   Engine target:
    //   { "macro": 1, "engineId": "OddfeliX", "parameterId": "snap_morph",
    //     "min": 0.0, "max": 1.0, "inverted": false }
    //
    //   Coupling route target:
    //   { "macro": 3, "coupling": true, "routeSlot": 0,
    //     "min": 0.0, "max": 1.0 }
    //
    void loadFromPreset(const juce::StringArray& macroLabels, const juce::var& macroTargetsVar)
    {
        // Reset everything
        clearAllTargets();

        // Apply labels (fall back to defaults if fewer than 4 provided)
        {
            const std::lock_guard<std::mutex> lock(labelMutex_);
            for (int i = 0; i < NumMacros; ++i)
            {
                if (i < macroLabels.size() && macroLabels[i].isNotEmpty())
                    labels[static_cast<size_t>(i)] = macroLabels[i];
                else
                    labels[static_cast<size_t>(i)] = DefaultLabels[static_cast<size_t>(i)];
            }
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
            target.engineId = entry.getProperty("engineId", "").toString();
            target.parameterId = entry.getProperty("parameterId", "").toString();
            target.minValue = static_cast<float>(entry.getProperty("min", 0.0));
            target.maxValue = static_cast<float>(entry.getProperty("max", 1.0));
            target.inverted = static_cast<bool>(entry.getProperty("inverted", false));

            // Coupling route targets: { "coupling": true, "routeSlot": 0-3 }
            target.isCouplingTarget = static_cast<bool>(entry.getProperty("coupling", false));
            target.couplingRouteSlot = static_cast<int>(entry.getProperty("routeSlot", -1));

            // Validate: coupling targets need a valid route slot; regular
            // targets need a non-empty parameterId.
            if (target.isCouplingTarget)
            {
                if (isValidCouplingSlot(target.couplingRouteSlot))
                    grouped[static_cast<size_t>(macroIdx)].push_back(std::move(target));
                else
                    DBG("MacroSystem: Dropping invalid coupling target — routeSlot " +
                        juce::String(target.couplingRouteSlot));
            }
            else if (target.parameterId.isNotEmpty())
            {
                grouped[static_cast<size_t>(macroIdx)].push_back(std::move(target));
            }
            else
            {
                DBG("MacroSystem: Dropping target with empty parameterId for macro " + juce::String(macroIdx + 1));
            }
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
    // Lock-free double-buffer: `buffers[0]` and `buffers[1]` are two complete
    // copies of the target list.  `activeBuffer` (0 or 1) indicates which copy
    // the audio thread should read.
    //
    // Message thread writes to `buffers[1 - activeBuffer]`, then flips
    // `activeBuffer` with a release store.  Audio thread loads `activeBuffer`
    // with acquire semantics and reads the corresponding buffer — no spinning,
    // no blocking, no dropped updates.
    //
    // `dirty` is set to true after each flip so callers can detect changes.
    // processBlock() clears it on each block; it is informational only.
    struct TargetBuffer
    {
        std::vector<MacroTarget>        targets;
        // Parallel array: cachedParams[i] is the raw pointer for targets[i].
        // May contain nullptrs if target parameter doesn't exist in APVTS.
        std::vector<std::atomic<float>*> cachedParams;
    };

    struct MacroSlot
    {
        std::array<TargetBuffer, 2> buffers;     // double-buffer
        std::atomic<int>            activeBuffer{0};
        std::atomic<bool>           dirty{false};
    };

    static bool isValidIndex(int idx) { return idx >= 0 && idx < NumMacros; }
    static bool isValidCouplingSlot(int slot) { return slot >= 0 && slot < NumCouplingRoutes; }

    juce::AudioProcessorValueTreeState& apvtsRef;

    // Cached raw parameter pointers for "macro1" through "macro4"
    std::array<std::atomic<float>*, NumMacros> macroParams = {};

    // Cached raw parameter pointers for coupling route amounts
    // ("cp_r1_amount" through "cp_r4_amount"). May contain nullptrs if
    // coupling performance params haven't been registered in the APVTS yet.
    std::array<std::atomic<float>*, NumCouplingRoutes> couplingAmountParams = {};

    // Smoothed macro values to eliminate zipper noise (~20ms ramp)
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, NumMacros> smoothedValues;

    // Mutex protecting labels[] — labels are UI-only and never touched on the
    // audio thread, so a std::mutex is sufficient. `mutable` so getLabel() and
    // getState() (both const) can lock it. (#682)
    mutable std::mutex labelMutex_;

    // Per-macro label (default: CHARACTER, MOVEMENT, COUPLING, SPACE)
    std::array<juce::String, NumMacros> labels;

    // Per-macro target slots
    std::array<MacroSlot, NumMacros> macroSlots;
};

} // namespace xoceanus
