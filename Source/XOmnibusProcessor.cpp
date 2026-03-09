#include "XOmnibusProcessor.h"
#include "Engines/Snap/SnapEngine.h"
#include "Engines/Morph/MorphEngine.h"
#include "Engines/Dub/DubEngine.h"
#include "Engines/Drift/DriftEngine.h"
#include "Engines/Bob/BobEngine.h"
#include "Engines/Fat/FatEngine.h"
#include "Engines/Onset/OnsetEngine.h"

// Register engines with their canonical IDs (matching getEngineId() return values)
static bool registered_Snap = xomnibus::EngineRegistry::instance().registerEngine(
    "Snap", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::SnapEngine>();
    });
static bool registered_Morph = xomnibus::EngineRegistry::instance().registerEngine(
    "Morph", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::MorphEngine>();
    });
static bool registered_Dub = xomnibus::EngineRegistry::instance().registerEngine(
    "Dub", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::DubEngine>();
    });
static bool registered_Drift = xomnibus::EngineRegistry::instance().registerEngine(
    "Drift", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::DriftEngine>();
    });
static bool registered_Bob = xomnibus::EngineRegistry::instance().registerEngine(
    "Bob", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::BobEngine>();
    });
static bool registered_Fat = xomnibus::EngineRegistry::instance().registerEngine(
    "Fat", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::FatEngine>();
    });
static bool registered_Onset = xomnibus::EngineRegistry::instance().registerEngine(
    "Onset", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OnsetEngine>();
    });

namespace xomnibus {

XOmnibusProcessor::XOmnibusProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "XOmnibusParams", createParameterLayout())
{
}

XOmnibusProcessor::~XOmnibusProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
    XOmnibusProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("masterVolume", 1), "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    // 4 Macro knobs (CHARACTER, MOVEMENT, COUPLING, SPACE)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("macro1", 1), "CHARACTER",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("macro2", 1), "MOVEMENT",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("macro3", 1), "COUPLING",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("macro4", 1), "SPACE",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Merge engine parameters directly.
    // Each engine adds its namespaced parameters to the shared vector.
    SnapEngine::addParameters(params);
    MorphEngine::addParameters(params);
    DubEngine::addParameters(params);
    DriftEngine::addParameters(params);
    BobEngine::addParameters(params);
    FatEngine::addParameters(params);
    OnsetEngine::addParameters(params);

    return { params.begin(), params.end() };
}

void XOmnibusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    couplingMatrix.prepare(samplesPerBlock);

    for (auto& buf : engineBuffers)
        buf.setSize(2, samplesPerBlock);

    crossfadeBuffer.setSize(2, samplesPerBlock);

    for (int i = 0; i < MaxSlots; ++i)
    {
        auto eng = std::atomic_load(&engines[i]);
        if (eng)
            eng->prepare(sampleRate, samplesPerBlock);
    }
}

void XOmnibusProcessor::releaseResources()
{
    for (int i = 0; i < MaxSlots; ++i)
    {
        auto eng = std::atomic_load(&engines[i]);
        if (eng)
            eng->releaseResources();
    }
}

void XOmnibusProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    buffer.clear();

    // Build engine pointer array for coupling matrix (atomic reads)
    std::array<SynthEngine*, MaxSlots> enginePtrs = {};
    std::array<std::shared_ptr<SynthEngine>, MaxSlots> engineRefs;  // prevent deletion during block
    int activeCount = 0;

    for (int i = 0; i < MaxSlots; ++i)
    {
        engineRefs[i] = std::atomic_load(&engines[i]);
        enginePtrs[i] = engineRefs[i].get();
        if (enginePtrs[i])
            ++activeCount;
    }

    if (activeCount == 0)
        return;

    couplingMatrix.setEngines(enginePtrs);

    // Render each active engine into its own buffer
    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;

        engineBuffers[i].clear();
        enginePtrs[i]->renderBlock(engineBuffers[i], midi, numSamples);
    }

    // Apply coupling matrix between engines
    couplingMatrix.processBlock(numSamples);

    // Mix all engine outputs to master
    const float masterVol = apvts.getRawParameterValue("masterVolume")->load();

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, engineBuffers[i], ch, 0, numSamples, masterVol);
    }

    // Process crossfade tails for outgoing engines
    for (int i = 0; i < MaxSlots; ++i)
    {
        auto& cf = crossfades[i];
        if (cf.fadeSamplesRemaining <= 0 || !cf.outgoing)
            continue;

        crossfadeBuffer.clear();
        juce::MidiBuffer emptyMidi;
        cf.outgoing->renderBlock(crossfadeBuffer, emptyMidi, numSamples);

        int fadeSamples = std::min(numSamples, cf.fadeSamplesRemaining);
        if (fadeSamples <= 0)
            continue;
        float fadeStep = cf.fadeGain / static_cast<float>(fadeSamples);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float gain = cf.fadeGain;
            auto* dest = buffer.getWritePointer(ch);
            auto* src = crossfadeBuffer.getReadPointer(ch);
            for (int s = 0; s < fadeSamples; ++s)
            {
                dest[s] += src[s] * gain * masterVol;
                gain -= fadeStep;
            }
        }

        cf.fadeGain -= fadeStep * static_cast<float>(fadeSamples);
        cf.fadeSamplesRemaining -= fadeSamples;

        if (cf.fadeSamplesRemaining <= 0)
        {
            cf.outgoing.reset();
            cf.fadeGain = 0.0f;
        }
    }
}

void XOmnibusProcessor::loadEngine(int slot, const std::string& engineId)
{
    if (slot < 0 || slot >= MaxSlots)
        return;

    auto newEngine = EngineRegistry::instance().createEngine(engineId);
    if (!newEngine)
        return;

    newEngine->attachParameters(apvts);
    newEngine->prepare(currentSampleRate, currentBlockSize);

    // Move the old engine to crossfade-out state
    auto oldEngine = std::atomic_load(&engines[slot]);
    if (oldEngine)
    {
        crossfades[slot].outgoing = oldEngine;
        crossfades[slot].fadeGain = 1.0f;
        crossfades[slot].fadeSamplesRemaining =
            static_cast<int>(currentSampleRate * CrossfadeMs * 0.001);
    }

    // Atomic swap — audio thread sees the new engine on next block
    auto shared = std::shared_ptr<SynthEngine>(std::move(newEngine));
    std::atomic_store(&engines[slot], shared);
}

void XOmnibusProcessor::unloadEngine(int slot)
{
    if (slot < 0 || slot >= MaxSlots)
        return;

    auto oldEngine = std::atomic_load(&engines[slot]);
    if (oldEngine)
    {
        crossfades[slot].outgoing = oldEngine;
        crossfades[slot].fadeGain = 1.0f;
        crossfades[slot].fadeSamplesRemaining =
            static_cast<int>(currentSampleRate * CrossfadeMs * 0.001);
    }

    std::shared_ptr<SynthEngine> empty;
    std::atomic_store(&engines[slot], empty);
}

SynthEngine* XOmnibusProcessor::getEngine(int slot) const
{
    if (slot >= 0 && slot < MaxSlots)
    {
        auto eng = std::atomic_load(&engines[slot]);
        return eng.get();
    }
    return nullptr;
}

juce::AudioProcessorEditor* XOmnibusProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void XOmnibusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml)
        copyXmlToBinary(*xml, destData);
}

void XOmnibusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

} // namespace xomnibus

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new xomnibus::XOmnibusProcessor();
}
