#include "XOmnibusProcessor.h"
#include "UI/XOmnibusEditor.h"
#include "Engines/Snap/SnapEngine.h"
#include "Engines/Morph/MorphEngine.h"
#include "Engines/Dub/DubEngine.h"
#include "Engines/Drift/DriftEngine.h"
#include "Engines/Bob/BobEngine.h"
#include "Engines/Fat/FatEngine.h"
#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Overworld/OverworldEngine.h"
#include "Engines/Opal/OpalEngine.h"
#include "Engines/Bite/BiteEngine.h"
#include "Engines/Organon/OrganonEngine.h"
#include "Engines/Ocelot/OcelotEngine.h"

// Register engines with their canonical IDs (matching getEngineId() return values).
// These MUST match the string returned by each engine's getEngineId().
// Legacy names ("Snap", "Morph", etc.) are resolved by resolveEngineAlias() in PresetManager.h.
static bool registered_OddfeliX = xomnibus::EngineRegistry::instance().registerEngine(
    "OddfeliX", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::SnapEngine>();
    });
static bool registered_OddOscar = xomnibus::EngineRegistry::instance().registerEngine(
    "OddOscar", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::MorphEngine>();
    });
static bool registered_Overdub = xomnibus::EngineRegistry::instance().registerEngine(
    "Overdub", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::DubEngine>();
    });
static bool registered_Odyssey = xomnibus::EngineRegistry::instance().registerEngine(
    "Odyssey", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::DriftEngine>();
    });
static bool registered_Oblong = xomnibus::EngineRegistry::instance().registerEngine(
    "Oblong", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::BobEngine>();
    });
static bool registered_Obese = xomnibus::EngineRegistry::instance().registerEngine(
    "Obese", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::FatEngine>();
    });
static bool registered_Onset = xomnibus::EngineRegistry::instance().registerEngine(
    "Onset", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OnsetEngine>();
    });
static bool registered_Overworld = xomnibus::EngineRegistry::instance().registerEngine(
    "Overworld", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OverworldEngine>();
    });
static bool registered_Opal = xomnibus::EngineRegistry::instance().registerEngine(
    "Opal", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OpalEngine>();
    });
static bool registered_Bite = xomnibus::EngineRegistry::instance().registerEngine(
    "Bite", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::BiteEngine>();
    });
static bool registered_Organon = xomnibus::EngineRegistry::instance().registerEngine(
    "Organon", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OrganonEngine>();
    });
static bool registered_Ocelot = xomnibus::EngineRegistry::instance().registerEngine(
    "Ocelot", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xocelot::OcelotEngine>();
    });

namespace xomnibus {

XOmnibusProcessor::XOmnibusProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "XOmnibusParams", createParameterLayout())
{
    cacheParameterPointers();
}

XOmnibusProcessor::~XOmnibusProcessor() = default;

void XOmnibusProcessor::cacheParameterPointers()
{
    cachedParams.masterVolume    = apvts.getRawParameterValue("masterVolume");
    cachedParams.cmEnabled       = apvts.getRawParameterValue("cm_enabled");
    cachedParams.cmPalette       = apvts.getRawParameterValue("cm_palette");
    cachedParams.cmVoicing       = apvts.getRawParameterValue("cm_voicing");
    cachedParams.cmSpread        = apvts.getRawParameterValue("cm_spread");
    cachedParams.cmSeqRunning    = apvts.getRawParameterValue("cm_seq_running");
    cachedParams.cmSeqBpm        = apvts.getRawParameterValue("cm_seq_bpm");
    cachedParams.cmSeqSwing      = apvts.getRawParameterValue("cm_seq_swing");
    cachedParams.cmSeqGate       = apvts.getRawParameterValue("cm_seq_gate");
    cachedParams.cmSeqPattern    = apvts.getRawParameterValue("cm_seq_pattern");
    cachedParams.cmVelCurve      = apvts.getRawParameterValue("cm_vel_curve");
    cachedParams.cmHumanize      = apvts.getRawParameterValue("cm_humanize");
    cachedParams.cmSidechainDuck = apvts.getRawParameterValue("cm_sidechain_duck");
    cachedParams.cmEnoMode       = apvts.getRawParameterValue("cm_eno_mode");
}

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
    OverworldEngine::addParameters(params);
    OpalEngine::addParameters(params);
    BiteEngine::addParameters(params);
    OrganonEngine::addParameters(params);

    // Chord Machine parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("cm_enabled", 1), "Chord Machine",
        false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_palette", 1), "CM Palette",
        juce::StringArray{ "WARM", "BRIGHT", "TENSION", "OPEN",
                           "DARK", "SWEET", "COMPLEX", "RAW" },
        0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_voicing", 1), "CM Voicing",
        juce::StringArray{ "ROOT-SPREAD", "DROP-2", "QUARTAL",
                           "UPPER STRUCT", "UNISON" },
        0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_spread", 1), "CM Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("cm_seq_running", 1), "CM Sequencer",
        false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_seq_bpm", 1), "CM BPM",
        juce::NormalisableRange<float>(30.0f, 300.0f, 0.1f), 122.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_seq_swing", 1), "CM Swing",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_seq_gate", 1), "CM Gate",
        juce::NormalisableRange<float>(0.01f, 1.0f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_seq_pattern", 1), "CM Pattern",
        juce::StringArray{ "FOUR", "OFF-BEAT", "SYNCO", "STAB",
                           "GATE", "PULSE", "BROKEN", "REST" },
        1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_vel_curve", 1), "CM Velocity Curve",
        juce::StringArray{ "EQUAL", "ROOT HEAVY", "TOP BRIGHT", "V-SHAPE" },
        1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_humanize", 1), "CM Humanize",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_sidechain_duck", 1), "CM Sidechain Duck",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("cm_eno_mode", 1), "CM Eno Mode",
        false));

    // Master FX parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_satDrive", 1), "Master Sat Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbSize", 1), "Master Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbMix", 1), "Master Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_compRatio", 1), "Master Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.0f, 0.4f), 2.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_compAttack", 1), "Master Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.0f, 0.4f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_compRelease", 1), "Master Comp Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 0.0f, 0.4f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_compMix", 1), "Master Comp Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    return { params.begin(), params.end() };
}

void XOmnibusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    couplingMatrix.prepare(samplesPerBlock);
    chordMachine.prepare(sampleRate, samplesPerBlock);
    masterFX.prepare(sampleRate, samplesPerBlock, apvts);

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
    masterFX.reset();
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

    // Sync Chord Machine state from cached parameter pointers (no hash lookups)
    chordMachine.setEnabled(cachedParams.cmEnabled->load() >= 0.5f);
    chordMachine.setPalette(static_cast<PaletteType>(
        static_cast<int>(cachedParams.cmPalette->load())));
    chordMachine.setVoicing(static_cast<VoicingMode>(
        static_cast<int>(cachedParams.cmVoicing->load())));
    chordMachine.setSpread(cachedParams.cmSpread->load());
    chordMachine.setSequencerRunning(cachedParams.cmSeqRunning->load() >= 0.5f);
    chordMachine.setBPM(cachedParams.cmSeqBpm->load());
    chordMachine.setSwing(cachedParams.cmSeqSwing->load());
    chordMachine.setGlobalGate(cachedParams.cmSeqGate->load());
    chordMachine.setVelocityCurve(static_cast<VelocityCurve>(
        static_cast<int>(cachedParams.cmVelCurve->load())));
    chordMachine.setHumanize(cachedParams.cmHumanize->load());
    chordMachine.setSidechainDuck(cachedParams.cmSidechainDuck->load());
    chordMachine.setEnoMode(cachedParams.cmEnoMode->load() >= 0.5f);

    // DAW host transport sync
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto ppq = pos->getPpqPosition())
            {
                double hostBPM = 122.0;
                if (auto bpmOpt = pos->getBpm())
                    hostBPM = *bpmOpt;
                bool hostPlaying = pos->getIsPlaying();
                chordMachine.syncToHost(*ppq, hostBPM, hostPlaying);
            }
        }
    }

    // Route MIDI through ChordMachine → 4 per-slot MidiBuffers.
    // When disabled, each slot gets a copy of the input MIDI (previous behavior).
    // When enabled, each slot gets its own chord-distributed note.
    chordMachine.processBlock(midi, slotMidi, numSamples);

    // Render each active engine into its own buffer using slot-specific MIDI
    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;

        engineBuffers[i].clear();
        enginePtrs[i]->renderBlock(engineBuffers[i], slotMidi[i], numSamples);
    }

    // Apply coupling matrix between engines.
    // Routes are loaded once here to avoid repeated atomic ref-count operations
    // inside processBlock (each atomic_load on a shared_ptr costs a LOCK prefix).
    auto couplingRoutes = couplingMatrix.loadRoutes();
    couplingMatrix.processBlock(numSamples, couplingRoutes);

    // Mix all engine outputs to master
    const float masterVol = cachedParams.masterVolume->load();

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
        // Distribute fade over ALL remaining samples, not just this block,
        // so the crossfade takes the full 50ms regardless of block size.
        float fadeStep = cf.fadeGain / static_cast<float>(cf.fadeSamplesRemaining);

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

    // Master FX chain: saturation → reverb → compression (post all engines + crossfades)
    masterFX.processBlock(buffer, numSamples);
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

    if (onEngineChanged)
        juce::MessageManager::callAsync([this, slot]{ if (onEngineChanged) onEngineChanged(slot); });
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

    if (onEngineChanged)
        juce::MessageManager::callAsync([this, slot]{ if (onEngineChanged) onEngineChanged(slot); });
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
    return new XOmnibusEditor(*this);
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

void XOmnibusProcessor::applyPreset(const PresetData& preset)
{
    // Each engine's params are stored under its engine name key.
    // Inner param names may be full APVTS IDs (e.g. "opal_source") or
    // unprefixed names (e.g. "source") — try both.
    for (const auto& [engineName, paramsVar] : preset.parametersByEngine)
    {
        auto* obj = paramsVar.getDynamicObject();
        if (!obj)
            continue;

        juce::String prefix = engineName.toLowerCase() + "_";

        for (const auto& prop : obj->getProperties())
        {
            juce::String fullId = prop.name.toString();

            // Try as-is first (already a full ID like "opal_source"),
            // then with prefix ("source" → "opal_source").
            if (apvts.getParameter(fullId) == nullptr)
                fullId = prefix + prop.name.toString();

            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(
                              apvts.getParameter(fullId)))
            {
                float val = static_cast<float>(prop.value);
                p->setValueNotifyingHost(p->convertTo0to1(val));
            }
        }
    }

    // Restore Chord Machine state if present in the preset.
    // The chordMachine data is stored in the sequencerData field (raw JSON var).
    if (!preset.sequencerData.isVoid() && preset.sequencerData.isObject())
    {
        chordMachine.restoreState(preset.sequencerData);

        // Sync the restored CM state back to APVTS so the UI reflects it
        auto syncParam = [&](const char* id, float val) {
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        };
        auto syncBool = [&](const char* id, bool val) {
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
                p->setValueNotifyingHost(val ? 1.0f : 0.0f);
        };

        syncBool("cm_enabled", chordMachine.isEnabled());
        syncParam("cm_palette", static_cast<float>(chordMachine.getPalette()));
        syncParam("cm_voicing", static_cast<float>(chordMachine.getVoicing()));
        syncParam("cm_spread", chordMachine.getSpread());
        syncBool("cm_seq_running", chordMachine.isSequencerRunning());
        syncParam("cm_seq_bpm", chordMachine.getBPM());
        syncParam("cm_seq_swing", chordMachine.getSwing());
        syncParam("cm_seq_gate", chordMachine.getGlobalGate());
        syncParam("cm_seq_pattern", static_cast<float>(chordMachine.getPattern()));
        syncParam("cm_vel_curve", static_cast<float>(chordMachine.getVelocityCurve()));
        syncParam("cm_humanize", chordMachine.getHumanize());
        syncParam("cm_sidechain_duck", chordMachine.getSidechainDuck());
        syncBool("cm_eno_mode", chordMachine.isEnoMode());
    }
}

} // namespace xomnibus

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new xomnibus::XOmnibusProcessor();
}
