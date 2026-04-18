// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// ObrixParamStore — Owns 82 std::atomic<float> values for all Obrix parameters.
// Populates a juce::AudioProcessorValueTreeState shim so that
// ObrixEngine::attachParameters() can wire its cached pointers.
#pragma once

#include "juce_compat.h"
#include <array>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

namespace obrix
{

struct ParamDef
{
    const char* id;
    float defaultValue;
};

class ObrixParamStore
{
public:
    ObrixParamStore();

    /// Wire all parameter atomics into the APVTS shim.
    void populateAPVTS(juce::AudioProcessorValueTreeState& apvts);

    /// Set a parameter by string ID. Thread-safe.
    bool setParameter(const std::string& id, float value);

    /// Get a parameter by string ID.
    float getParameter(const std::string& id) const;

    /// Get all parameter IDs.
    const std::vector<ParamDef>& getParamDefs() const { return paramDefs_; }

    /// Number of parameters.
    int size() const { return static_cast<int>(paramDefs_.size()); }

private:
    std::vector<ParamDef> paramDefs_;
    std::vector<std::atomic<float>> values_;
    std::unordered_map<std::string, int> idToIndex_;
};

} // namespace obrix
