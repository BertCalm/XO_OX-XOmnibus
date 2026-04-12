// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// ObrixSDKAdapter implementation — lives in the JUCE-shimmed world.
// This is the ONLY file that includes ObrixEngine.h.
// The public header (ObrixSDKAdapter.h) has zero JUCE/SDK dependencies.

#include "obrix/ObrixSDKAdapter.h"
#include "obrix/ObrixParamStore.h"

// These pull in the JUCE-shimmed SynthEngine.h via the include path
#include "Engines/Obrix/ObrixEngine.h"

#include <cmath>
#include <cstring>

namespace obrix
{

static constexpr int kMaxBlockSize = 2048;

struct ObrixSDKAdapter::Impl
{
    xoceanus::ObrixEngine engine;
    ObrixParamStore paramStore;
    juce::AudioProcessorValueTreeState apvts;

    // Pre-allocated conversion buffers
    float leftBuf[kMaxBlockSize] = {};
    float rightBuf[kMaxBlockSize] = {};
    float* channelPtrs[2] = {leftBuf, rightBuf};
    juce::AudioBuffer<float> juceBuffer;
    juce::MidiBuffer juceMidi;

    bool prepared = false;

    Impl()
    {
        // Wire parameter atomics into the APVTS shim
        paramStore.populateAPVTS(apvts);

        // Let the engine cache its raw parameter pointers
        engine.attachParameters(apvts);

        // Set up the JUCE AudioBuffer shim to reference our pre-allocated arrays
        juceBuffer.setDataToReferTo(channelPtrs, 2, kMaxBlockSize);
    }
};

ObrixSDKAdapter::ObrixSDKAdapter()
    : impl_(std::make_unique<Impl>())
{
}

ObrixSDKAdapter::~ObrixSDKAdapter() = default;

void ObrixSDKAdapter::prepare(double sampleRate, int maxBlockSize)
{
    if (maxBlockSize > kMaxBlockSize)
        maxBlockSize = kMaxBlockSize;

    impl_->engine.prepare(sampleRate, maxBlockSize);
    impl_->engine.prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    impl_->prepared = true;
}

void ObrixSDKAdapter::reset()
{
    impl_->engine.reset();
}

void ObrixSDKAdapter::renderBlock(float* left, float* right, int numSamples,
                                   const MidiEvent* midiEvents, int numMidiEvents)
{
    if (!impl_->prepared || numSamples <= 0 || numSamples > kMaxBlockSize)
        return;

    // Clear our internal buffers (ObrixEngine uses addSample)
    std::memset(impl_->leftBuf, 0, sizeof(float) * static_cast<size_t>(numSamples));
    std::memset(impl_->rightBuf, 0, sizeof(float) * static_cast<size_t>(numSamples));

    // Update the JUCE buffer shim to the correct size
    impl_->juceBuffer.setDataToReferTo(impl_->channelPtrs, 2, numSamples);

    // Convert MidiEvents to JUCE MidiBuffer shim
    impl_->juceMidi.clear();
    if (midiEvents != nullptr)
    {
        for (int i = 0; i < numMidiEvents; ++i)
        {
            const auto& ev = midiEvents[i];
            if (ev.numBytes == 3)
                impl_->juceMidi.addEvent(
                    juce::MidiMessage(ev.data[0], ev.data[1], ev.data[2]),
                    ev.sampleOffset);
            else if (ev.numBytes == 2)
                impl_->juceMidi.addEvent(
                    juce::MidiMessage(ev.data[0], ev.data[1]),
                    ev.sampleOffset);
        }
    }

    // Render
    impl_->engine.renderBlock(impl_->juceBuffer, impl_->juceMidi, numSamples);

    // Copy results to caller's buffers
    if (left)
        for (int s = 0; s < numSamples; ++s)
            left[s] += impl_->leftBuf[s];
    if (right)
        for (int s = 0; s < numSamples; ++s)
            right[s] += impl_->rightBuf[s];

    // NaN/Inf safety clamp
    for (int s = 0; s < numSamples; ++s)
    {
        if (left && !std::isfinite(left[s]))   left[s] = 0.0f;
        if (right && !std::isfinite(right[s])) right[s] = 0.0f;
    }
}

bool ObrixSDKAdapter::setParameter(const std::string& id, float value)
{
    return impl_->paramStore.setParameter(id, value);
}

float ObrixSDKAdapter::getParameter(const std::string& id) const
{
    return impl_->paramStore.getParameter(id);
}

std::vector<ParamInfo> ObrixSDKAdapter::getParameterList() const
{
    std::vector<ParamInfo> result;
    for (const auto& pd : impl_->paramStore.getParamDefs())
        result.push_back({pd.id, pd.defaultValue});
    return result;
}

int ObrixSDKAdapter::getParameterCount() const
{
    return impl_->paramStore.size();
}

int ObrixSDKAdapter::getActiveVoiceCount() const
{
    return impl_->engine.getActiveVoiceCount();
}

} // namespace obrix
