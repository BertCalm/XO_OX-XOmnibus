// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// OrtolanEngine — Stub (engine not yet implemented).
#include "../../Core/SynthEngine.h"

namespace xoceanus
{

class OrtolanEngine : public SynthEngine
{
public:
    void prepare(double /*sampleRate*/, int /*maxBlockSize*/) override {}
    void releaseResources() override {}
    void reset() override {}
    void renderBlock(juce::AudioBuffer<float>& /*buffer*/, juce::MidiBuffer& /*midi*/, int /*numSamples*/) override {}
    float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override { return 0.0f; }
    void applyCouplingInput(CouplingType /*type*/, float /*amount*/, const float* /*sourceBuffer*/, int /*numSamples*/) override {}
    void attachParameters(juce::AudioProcessorValueTreeState& /*apvts*/) override {}
    juce::String getEngineId() const override { return "Ortolan"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF8B9E6B); }
    int getMaxVoices() const override { return 8; }
};

} // namespace xoceanus
