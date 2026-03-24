#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <unordered_map>
#include <vector>
#include <functional>

namespace xolokun {

//==============================================================================
// MIDILearnManager — Cross-platform MIDI controller mapping.
//
// Shared between desktop and mobile. No platform-specific code.
//
// Workflow:
//   1. User long-presses (mobile) or right-clicks (desktop) a parameter
//   2. enterLearnMode(paramId) is called — manager listens for next CC
//   3. User moves a MIDI controller
//   4. processMidi() captures the CC and creates a mapping
//   5. Subsequent CC messages from that controller update the parameter
//
// Storage:
//   - Global mappings persist in app settings (JSON)
//   - Per-preset mappings are serialized with the preset state
//   - User chooses scope via "Store mappings per preset" setting
//
// Thread safety:
//   - enterLearnMode() / exitLearnMode() on message thread
//   - processMidi() on audio thread (reads mappings, writes parameter values)
//   - Mapping mutations happen on message thread with lock-free swap
//
class MIDILearnManager {
public:
    struct MIDIMapping {
        int ccNumber;                   // 0-127
        int channel;                    // 0-15 (0 = omni — responds to any channel)
        juce::String paramId;           // APVTS parameter ID (e.g., "snap_filterCutoff")
        float rangeMin = 0.0f;          // Output range minimum
        float rangeMax = 1.0f;          // Output range maximum
        bool isPerPreset = false;       // true = saved with preset, false = global
    };

    using LearnCompleteCallback = std::function<void(const juce::String& paramId, int ccNumber)>;

    void setAPVTS(juce::AudioProcessorValueTreeState* apvts) { this->apvts = apvts; }

    //-- Learn mode (message thread) ---------------------------------------------

    void enterLearnMode(const juce::String& paramId)
    {
        learnTargetParam = paramId;
        learning = true;
    }

    void exitLearnMode()
    {
        learnTargetParam = "";
        learning = false;
    }

    bool isLearning() const { return learning; }
    juce::String getLearningParam() const { return learnTargetParam; }

    void setLearnCompleteCallback(LearnCompleteCallback cb)
    {
        onLearnComplete = std::move(cb);
    }

    //-- Mapping management (message thread) -------------------------------------

    void addMapping(MIDIMapping mapping)
    {
        // Remove any existing mapping for this CC+channel combo
        removeMapping(mapping.ccNumber, mapping.channel);

        auto newMappings = std::make_shared<std::vector<MIDIMapping>>();
        auto current = std::atomic_load(&activeMappings);
        if (current)
            *newMappings = *current;
        newMappings->push_back(std::move(mapping));
        std::atomic_store(&activeMappings, newMappings);
    }

    void removeMapping(int ccNumber, int channel)
    {
        auto current = std::atomic_load(&activeMappings);
        if (!current || current->empty())
            return;

        auto newMappings = std::make_shared<std::vector<MIDIMapping>>(*current);
        newMappings->erase(
            std::remove_if(newMappings->begin(), newMappings->end(),
                [&](const MIDIMapping& m) {
                    return m.ccNumber == ccNumber
                        && (m.channel == channel || m.channel == 0 || channel == 0);
                }),
            newMappings->end());
        std::atomic_store(&activeMappings, newMappings);
    }

    void removeMappingForParam(const juce::String& paramId)
    {
        auto current = std::atomic_load(&activeMappings);
        if (!current || current->empty())
            return;

        auto newMappings = std::make_shared<std::vector<MIDIMapping>>(*current);
        newMappings->erase(
            std::remove_if(newMappings->begin(), newMappings->end(),
                [&](const MIDIMapping& m) { return m.paramId == paramId; }),
            newMappings->end());
        std::atomic_store(&activeMappings, newMappings);
    }

    void clearAllMappings()
    {
        std::atomic_store(&activeMappings,
            std::make_shared<std::vector<MIDIMapping>>());
    }

    std::vector<MIDIMapping> getMappings() const
    {
        auto m = std::atomic_load(&activeMappings);
        return m ? *m : std::vector<MIDIMapping>{};
    }

    bool hasMapping(const juce::String& paramId) const
    {
        auto m = std::atomic_load(&activeMappings);
        if (!m) return false;
        for (const auto& mapping : *m)
            if (mapping.paramId == paramId)
                return true;
        return false;
    }

    //-- Audio thread processing -------------------------------------------------

    // Process MIDI buffer for CC messages. Call once per audio callback.
    // Updates mapped parameters and handles learn mode capture.
    void processMidi(const juce::MidiBuffer& midi)
    {
        if (!apvts)
            return;

        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (!msg.isController())
                continue;

            const int cc = msg.getControllerNumber();
            const int ch = msg.getChannel();  // 1-16
            const float ccValue = static_cast<float>(msg.getControllerValue()) / 127.0f;

            // Learn mode: capture this CC for the target parameter
            if (learning && learnTargetParam.isNotEmpty())
            {
                // Defer the actual mapping creation to the message thread
                pendingLearnCC.store(cc, std::memory_order_relaxed);
                pendingLearnChannel.store(ch, std::memory_order_relaxed);
                continue;
            }

            // Apply active mappings
            auto mappings = std::atomic_load(&activeMappings);
            if (!mappings)
                continue;

            for (const auto& m : *mappings)
            {
                if (m.ccNumber != cc)
                    continue;
                if (m.channel != 0 && m.channel != ch)
                    continue;

                // Map CC value to parameter range
                float paramValue = m.rangeMin + ccValue * (m.rangeMax - m.rangeMin);

                // Audio-thread safe: setValueNotifyingHost() only.
                // beginChangeGesture/endChangeGesture are message-thread only
                // (they signal host automation recording) — calling them here
                // causes DAW crashes in Pro Tools, Logic, and Reaper.
                if (auto* p = apvts->getParameter(m.paramId))
                    p->setValueNotifyingHost(p->convertTo0to1(paramValue));
            }
        }
    }

    // Call from the message thread timer to finalize learn mode captures.
    // This bridges the audio thread capture to message thread mapping creation.
    void checkPendingLearn()
    {
        int cc = pendingLearnCC.exchange(-1, std::memory_order_relaxed);
        int ch = pendingLearnChannel.load(std::memory_order_relaxed);

        if (cc >= 0 && learning && learnTargetParam.isNotEmpty())
        {
            MIDIMapping mapping;
            mapping.ccNumber = cc;
            mapping.channel = ch;
            mapping.paramId = learnTargetParam;
            mapping.rangeMin = 0.0f;
            mapping.rangeMax = 1.0f;
            mapping.isPerPreset = perPresetDefault;

            addMapping(mapping);

            if (onLearnComplete)
                onLearnComplete(learnTargetParam, cc);

            exitLearnMode();
        }
    }

    //-- Serialization -----------------------------------------------------------

    juce::var toJSON() const
    {
        auto arr = juce::Array<juce::var>();
        auto mappings = getMappings();
        for (const auto& m : mappings)
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("cc", m.ccNumber);
            obj->setProperty("channel", m.channel);
            obj->setProperty("param", m.paramId);
            obj->setProperty("min", static_cast<double>(m.rangeMin));
            obj->setProperty("max", static_cast<double>(m.rangeMax));
            obj->setProperty("perPreset", m.isPerPreset);
            arr.add(juce::var(obj));
        }
        return arr;
    }

    void fromJSON(const juce::var& json)
    {
        clearAllMappings();
        if (auto* arr = json.getArray())
        {
            for (const auto& item : *arr)
            {
                if (auto* obj = item.getDynamicObject())
                {
                    MIDIMapping m;
                    m.ccNumber = static_cast<int>(obj->getProperty("cc"));
                    m.channel = static_cast<int>(obj->getProperty("channel"));
                    m.paramId = obj->getProperty("param").toString();
                    m.rangeMin = static_cast<float>(static_cast<double>(obj->getProperty("min")));
                    m.rangeMax = static_cast<float>(static_cast<double>(obj->getProperty("max")));
                    m.isPerPreset = static_cast<bool>(obj->getProperty("perPreset"));
                    addMapping(m);
                }
            }
        }
    }

    //-- Default CC mappings (per spec) ------------------------------------------

    void loadDefaultMappings()
    {
        // CC1 Mod Wheel → macro2: MOVEMENT
        addMapping({ 1, 0, "macro2", 0.0f, 1.0f, false });
        // CC2 Breath → macro1: CHARACTER
        addMapping({ 2, 0, "macro1", 0.0f, 1.0f, false });
        // CC11 Expression → macro4: SPACE
        addMapping({ 11, 0, "macro4", 0.0f, 1.0f, false });
        // CC74 Brightness → macro1: CHARACTER (filter brightness proxy)
        addMapping({ 74, 0, "macro1", 0.0f, 1.0f, false });

        // Coupling performance params (spec §6.1 — all undefined/unassigned CCs)
        // Amount params are bipolar (-1.0 to 1.0); CC centre (64/127 ≈ 0.504) maps to ~0.0 depth.
        // CC3 → cp_r1_amount: coupling route 1 depth
        addMapping({ 3, 0, "cp_r1_amount", -1.0f, 1.0f, false });
        // CC9 → cp_r2_amount: coupling route 2 depth
        addMapping({ 9, 0, "cp_r2_amount", -1.0f, 1.0f, false });
        // CC14 → cp_r3_amount: coupling route 3 depth
        addMapping({ 14, 0, "cp_r3_amount", -1.0f, 1.0f, false });
        // CC15 → cp_r4_amount: coupling route 4 depth
        addMapping({ 15, 0, "cp_r4_amount", -1.0f, 1.0f, false });
        // CC85 → cp_r1_type: coupling type sweep for route 1 (0 = AmpToFilter, 13 = KnotTopology)
        addMapping({ 85, 0, "cp_r1_type", 0.0f, 13.0f, false });
    }

    void setPerPresetDefault(bool perPreset) { perPresetDefault = perPreset; }

private:
    juce::AudioProcessorValueTreeState* apvts = nullptr;
    bool learning = false;
    juce::String learnTargetParam;
    bool perPresetDefault = false;
    LearnCompleteCallback onLearnComplete;

    std::shared_ptr<std::vector<MIDIMapping>> activeMappings =
        std::make_shared<std::vector<MIDIMapping>>();

    std::atomic<int> pendingLearnCC { -1 };
    std::atomic<int> pendingLearnChannel { 0 };
};

} // namespace xolokun
