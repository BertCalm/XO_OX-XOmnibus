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
#include "Engines/Ouroboros/OuroborosEngine.h"
#include "Engines/Obsidian/ObsidianEngine.h"
#include "Engines/Origami/OrigamiEngine.h"
#include "Engines/Oracle/OracleEngine.h"
#include "Engines/Obscura/ObscuraEngine.h"
#include "Engines/Oceanic/OceanicEngine.h"
#include "Engines/Optic/OpticEngine.h"
#include "Engines/Oblique/ObliqueEngine.h"
#include "Engines/Orbital/OrbitalEngine.h"
#include "Engines/Osprey/OspreyEngine.h"
#include "Engines/Osteria/OsteriaEngine.h"
#include "Engines/Owlfish/OwlfishEngine.h"
#include "Engines/Ohm/OhmEngine.h"
#include "Engines/Orphica/OrphicaEngine.h"
#include "Engines/Obbligato/ObbligatoEngine.h"
#include "Engines/Ottoni/OttoniEngine.h"
#include "Engines/Ole/OleEngine.h"
#include "Engines/Overlap/XOverlapAdapter.h"
#include "Engines/Outwit/XOutwitAdapter.h"
#include "Engines/Ombre/OmbreEngine.h"
#include "Engines/Orca/OrcaEngine.h"
#include "Engines/Octopus/OctopusEngine.h"

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
static bool registered_Ouroboros = xomnibus::EngineRegistry::instance().registerEngine(
    "Ouroboros", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OuroborosEngine>();
    });
static bool registered_Obsidian = xomnibus::EngineRegistry::instance().registerEngine(
    "Obsidian", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::ObsidianEngine>();
    });
static bool registered_Origami = xomnibus::EngineRegistry::instance().registerEngine(
    "Origami", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OrigamiEngine>();
    });
static bool registered_Oracle = xomnibus::EngineRegistry::instance().registerEngine(
    "Oracle", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OracleEngine>();
    });
static bool registered_Obscura = xomnibus::EngineRegistry::instance().registerEngine(
    "Obscura", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::ObscuraEngine>();
    });
static bool registered_Oceanic = xomnibus::EngineRegistry::instance().registerEngine(
    "Oceanic", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OceanicEngine>();
    });
static bool registered_Optic = xomnibus::EngineRegistry::instance().registerEngine(
    "Optic", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OpticEngine>();
    });
static bool registered_Oblique = xomnibus::EngineRegistry::instance().registerEngine(
    "Oblique", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::ObliqueEngine>();
    });
static bool registered_Orbital = xomnibus::EngineRegistry::instance().registerEngine(
    "Orbital", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OrbitalEngine>();
    });
static bool registered_Osprey = xomnibus::EngineRegistry::instance().registerEngine(
    "Osprey", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OspreyEngine>();
    });
static bool registered_Osteria = xomnibus::EngineRegistry::instance().registerEngine(
    "Osteria", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OsteriaEngine>();
    });
static bool registered_Owlfish = xomnibus::EngineRegistry::instance().registerEngine(
    "Owlfish", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xowlfish::OwlfishEngine>();
    });
static bool registered_Ohm = xomnibus::EngineRegistry::instance().registerEngine(
    "Ohm", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OhmEngine>();
    });
static bool registered_Orphica = xomnibus::EngineRegistry::instance().registerEngine(
    "Orphica", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OrphicaEngine>();
    });
static bool registered_Obbligato = xomnibus::EngineRegistry::instance().registerEngine(
    "Obbligato", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::ObbligatoEngine>();
    });
static bool registered_Ottoni = xomnibus::EngineRegistry::instance().registerEngine(
    "Ottoni", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OttoniEngine>();
    });
static bool registered_Ole = xomnibus::EngineRegistry::instance().registerEngine(
    "Ole", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OleEngine>();
    });
static bool registered_XOverlap = xomnibus::EngineRegistry::instance().registerEngine(
    "XOverlap", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::XOverlapEngine>();
    });
static bool registered_XOutwit = xomnibus::EngineRegistry::instance().registerEngine(
    "XOutwit", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::XOutwitEngine>();
    });
static bool registered_Ombre = xomnibus::EngineRegistry::instance().registerEngine(
    "Ombre", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OmbreEngine>();
    });
static bool registered_Orca = xomnibus::EngineRegistry::instance().registerEngine(
    "Orca", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OrcaEngine>();
    });
static bool registered_Octopus = xomnibus::EngineRegistry::instance().registerEngine(
    "Octopus", []() -> std::unique_ptr<xomnibus::SynthEngine> {
        return std::make_unique<xomnibus::OctopusEngine>();
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
    cachedParams.ohmCommune      = apvts.getRawParameterValue("ohm_macroCommune");
    cachedParams.obblBond        = apvts.getRawParameterValue("obbl_macroBond");
    cachedParams.oleDrama        = apvts.getRawParameterValue("ole_macroDrama");
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
    OuroborosEngine::addParameters(params);
    ObsidianEngine::addParameters(params);
    OrigamiEngine::addParameters(params);
    OracleEngine::addParameters(params);
    ObscuraEngine::addParameters(params);
    OceanicEngine::addParameters(params);
    OpticEngine::addParameters(params);
    ObliqueEngine::addParameters(params);
    OrbitalEngine::addParameters(params);
    OspreyEngine::addParameters(params);
    OsteriaEngine::addParameters(params);
    xocelot::addParameters(params);
    xowlfish::addParameters(params);

    // Constellation Family Engines (SP7)
    OhmEngine::addParameters(params);
    OrphicaEngine::addParameters(params);
    ObbligatoEngine::addParameters(params);
    OttoniEngine::addParameters(params);
    OleEngine::addParameters(params);

    OmbreEngine::addParameters(params);
    OrcaEngine::addParameters(params);
    OctopusEngine::addParameters(params);

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
    // Stage 1: Saturation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_satDrive", 1), "Master Sat Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_satMode", 1), "Master Sat Mode",
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 1.0f));

    // Stage 2: Corroder (digital erosion)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrMix", 1), "Master Corroder Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrBits", 1), "Master Corroder Bits",
        juce::NormalisableRange<float>(1.0f, 24.0f, 0.0f, 0.5f), 24.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrSR", 1), "Master Corroder SR",
        juce::NormalisableRange<float>(100.0f, 44100.0f, 0.0f, 0.3f), 44100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrFM", 1), "Master Corroder FM",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrTone", 1), "Master Corroder Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // Stage 4: Combulator (tuned comb bank)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combMix", 1), "Master Comb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combFreq", 1), "Master Comb Freq",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.0f, 0.3f), 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combFeedback", 1), "Master Comb Feedback",
        juce::NormalisableRange<float>(0.0f, 0.98f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combDamping", 1), "Master Comb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combNoise", 1), "Master Comb Noise",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combSpread", 1), "Master Comb Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combOffset2", 1), "Master Comb Offset 2",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 7.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combOffset3", 1), "Master Comb Offset 3",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 12.0f));

    // Stage 6: Frequency Shifter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_fshiftHz", 1), "Master Freq Shift Hz",
        juce::NormalisableRange<float>(-1000.0f, 1000.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_fshiftMix", 1), "Master Freq Shift Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_fshiftMode", 1), "Master Freq Shift Mode",
        juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_fshiftFeedback", 1), "Master Freq Shift FB",
        juce::NormalisableRange<float>(0.0f, 0.9f), 0.0f));

    // Stage 8: Multiband OTT
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_ottMix", 1), "Master OTT Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_ottDepth", 1), "Master OTT Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
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

    // Master FX: Delay parameters (Stage 2)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayTime", 1), "Master Delay Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.0f, 0.4f), 375.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayFeedback", 1), "Master Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayMix", 1), "Master Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayPingPong", 1), "Master Delay Ping Pong",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayDamping", 1), "Master Delay Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayDiffusion", 1), "Master Delay Diffusion",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delaySync", 1), "Master Delay Sync",
        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));

    // Master FX: Modulation parameters (Stage 4)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modRate", 1), "Master Mod Rate",
        juce::NormalisableRange<float>(0.05f, 10.0f, 0.0f, 0.4f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modDepth", 1), "Master Mod Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modMix", 1), "Master Mod Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modMode", 1), "Master Mod Mode",
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modFeedback", 1), "Master Mod Feedback",
        juce::NormalisableRange<float>(0.0f, 0.85f), 0.0f));

    // Master FX: Sequencer parameters (Stage 6)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqEnabled", 1), "Master Seq Enabled",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqRate", 1), "Master Seq Rate",
        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqSteps", 1), "Master Seq Steps",
        juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqDepth", 1), "Master Seq Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqSmooth", 1), "Master Seq Smooth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqTarget1", 1), "Master Seq Target 1",
        juce::NormalisableRange<float>(0.0f, 17.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqTarget2", 1), "Master Seq Target 2",
        juce::NormalisableRange<float>(0.0f, 17.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqPattern", 1), "Master Seq Pattern",
        juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqEnvFollow", 1), "Master Seq Env Follow",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqEnvAmount", 1), "Master Seq Env Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Stage 3: Vibe Knob (bipolar: -1 sweet, +1 grit)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_vibeAmount", 1), "Master Vibe",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

    // Stage 4: Spectral Tilt
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_tiltAmount", 1), "Master Tilt Amount",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_tiltMix", 1), "Master Tilt Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // Stage 4: Transient Designer
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_tdAttack", 1), "Master TD Attack",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_tdSustain", 1), "Master TD Sustain",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_tdMix", 1), "Master TD Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 7: Doppler Effect
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_dopplerDist", 1), "Master Doppler Distance",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_dopplerSpeed", 1), "Master Doppler Speed",
        juce::NormalisableRange<float>(0.01f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_dopplerMix", 1), "Master Doppler Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 11: Granular Smear
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_smearAmount", 1), "Master Smear Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_smearGrainSize", 1), "Master Smear Grain Size",
        juce::NormalisableRange<float>(10.0f, 200.0f, 0.0f, 0.5f), 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_smearDensity", 1), "Master Smear Density",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_smearMix", 1), "Master Smear Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 12: Harmonic Exciter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_excDrive", 1), "Master Exciter Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_excFreq", 1), "Master Exciter Freq",
        juce::NormalisableRange<float>(1000.0f, 12000.0f, 0.0f, 0.3f), 3500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_excTone", 1), "Master Exciter Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_excMix", 1), "Master Exciter Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 13: Stereo Sculptor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculLowWidth", 1), "Master Sculptor Low Width",
        juce::NormalisableRange<float>(0.0f, 2.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculMidWidth", 1), "Master Sculptor Mid Width",
        juce::NormalisableRange<float>(0.0f, 2.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculHighWidth", 1), "Master Sculptor High Width",
        juce::NormalisableRange<float>(0.0f, 2.0f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculLowCross", 1), "Master Sculptor Low Crossover",
        juce::NormalisableRange<float>(60.0f, 500.0f, 0.0f, 0.4f), 200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculHighCross", 1), "Master Sculptor High Crossover",
        juce::NormalisableRange<float>(2000.0f, 12000.0f, 0.0f, 0.4f), 4000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculMix", 1), "Master Sculptor Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 14: Psychoacoustic Width
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_pwidthAmount", 1), "Master PWidth Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_pwidthHaas", 1), "Master PWidth Haas",
        juce::NormalisableRange<float>(0.1f, 30.0f, 0.0f, 0.4f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_pwidthComb", 1), "Master PWidth Comb Freq",
        juce::NormalisableRange<float>(200.0f, 2000.0f, 0.0f, 0.4f), 600.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_pwidthMono", 1), "Master PWidth Mono Safe",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_pwidthMix", 1), "Master PWidth Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 20: Brickwall Limiter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_limCeiling", 1), "Master Limiter Ceiling",
        juce::NormalisableRange<float>(-6.0f, 0.0f), -0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_limRelease", 1), "Master Limiter Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 0.0f, 0.4f), 50.0f));

    // Stage 6: fXOsmosis (Membrane Transfer)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_osmMembrane", 1), "Master Osmosis Membrane",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_osmReactivity", 1), "Master Osmosis Reactivity",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_osmResonance", 1), "Master Osmosis Resonance",
        juce::NormalisableRange<float>(0.0f, 0.85f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_osmSaturation", 1), "Master Osmosis Saturation",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_osmMix", 1), "Master Osmosis Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 12: fXOneiric (Dream State)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onDelayTime", 1), "Master Oneiric Delay",
        juce::NormalisableRange<float>(1.0f, 1500.0f, 0.0f, 0.3f), 350.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onShiftHz", 1), "Master Oneiric Shift",
        juce::NormalisableRange<float>(-500.0f, 500.0f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onFeedback", 1), "Master Oneiric Feedback",
        juce::NormalisableRange<float>(0.0f, 0.92f), 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onDamping", 1), "Master Oneiric Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onSpread", 1), "Master Oneiric Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onMix", 1), "Master Oneiric Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    return { params.begin(), params.end() };
}

// SRO: Engine-specific silence gate hold times.
// Tuned to each engine's tail characteristics per SilenceGate.h guidance:
//   Percussive (fast decay, no internal reverb):           100ms
//   Standard (typical synth voices):                       200ms
//   Reverb-tail (internal delay/reverb/granular):          500ms
//   Infinite-sustain (self-sustaining feedback/metabolic): 1000ms
//   Visual-only (OPTIC — no audio output):                  50ms
static float silenceGateHoldMs(const juce::String& engineId)
{
    // Percussive — 100ms
    if (engineId == "Onset"     || engineId == "Bite"      || engineId == "OddfeliX"
     || engineId == "Origami")
        return 100.0f;

    // Reverb-tail / granular / delay — 500ms
    if (engineId == "Overdub"   || engineId == "Opal"      || engineId == "Oceanic"
     || engineId == "Obscura"   || engineId == "Osprey"    || engineId == "Osteria"
     || engineId == "Ombre"     || engineId == "Overlap")
        return 500.0f;

    // Infinite-sustain / self-exciting feedback — 1000ms
    if (engineId == "Organon"   || engineId == "Ouroboros"  || engineId == "Oracle"
     || engineId == "Owlfish")
        return 1000.0f;

    // Visual-only — 50ms (Optic generates no audio; gate closes fast)
    if (engineId == "Optic")
        return 50.0f;

    // Standard — 200ms (all remaining engines)
    return 200.0f;
}

void XOmnibusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    couplingMatrix.prepare(samplesPerBlock);
    chordMachine.prepare(sampleRate, samplesPerBlock);
    masterFX.prepare(sampleRate, samplesPerBlock, apvts);

    // SRO: Prepare profilers and auditor
    for (auto& prof : engineProfilers)
        prof.prepare(sampleRate, samplesPerBlock);
    sroAuditor.prepare(sampleRate, samplesPerBlock);

    for (auto& buf : engineBuffers)
        buf.setSize(2, samplesPerBlock);

    crossfadeBuffer.setSize(2, samplesPerBlock);

    for (int i = 0; i < MaxSlots; ++i)
    {
        auto eng = std::atomic_load(&engines[i]);
        if (eng)
        {
            eng->prepare(sampleRate, samplesPerBlock);
            eng->prepareSilenceGate(sampleRate, samplesPerBlock,
                                    silenceGateHoldMs(eng->getEngineId()));
        }
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

        // SRO: Wake silence gate on note-on events (BEFORE bypass check)
        for (const auto metadata : slotMidi[i])
        {
            if (metadata.getMessage().isNoteOn())
            {
                enginePtrs[i]->wakeSilenceGate();
                break;  // One wake per block is sufficient
            }
        }

        // SRO: Skip DSP if silence gate says engine output is silent
        engineBuffers[i].clear();
        if (enginePtrs[i]->isSilenceGateBypassed() && slotMidi[i].isEmpty())
        {
            // Still record as active-but-bypassed for the auditor
            sroAuditor.recordSlot(i, engineProfilers[i].getStats(), true);
            continue;
        }

        {
            EngineProfiler::ScopedMeasurement measurement (engineProfilers[i]);
            enginePtrs[i]->renderBlock(engineBuffers[i], slotMidi[i], numSamples);
        }

        // SRO: Feed output to silence gate for analysis
        enginePtrs[i]->analyzeForSilenceGate(engineBuffers[i], numSamples);

        // SRO: Record slot stats for auditor (CPU + silence gate state)
        sroAuditor.recordSlot(i, engineProfilers[i].getStats(),
                              enginePtrs[i]->isSilenceGateBypassed());
    }

    // Apply coupling matrix between engines.
    // Routes are loaded once here to avoid repeated atomic ref-count operations
    // inside processBlock (each atomic_load on a shared_ptr costs a LOCK prefix).
    auto couplingRoutes = couplingMatrix.loadRoutes();
    couplingMatrix.processBlock(numSamples, couplingRoutes);

    // Apply family bleed between Constellation engines (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE)
    processFamilyBleed(enginePtrs);

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

    // Master FX chain: sat → delay → reverb → mod → comp + sequencer (post all engines)
    double ppqPos = -1.0;
    double bpm = 0.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            if (posInfo->getPpqPosition().hasValue())
                ppqPos = *posInfo->getPpqPosition();
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();
        }
    }
    masterFX.processBlock(buffer, numSamples, ppqPos, bpm);
}

void XOmnibusProcessor::processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs)
{
    // Identify which slots hold Constellation family engines
    static const juce::StringArray kFamilyIds = { "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole" };

    // Collect family engine pointers and their slot indices
    struct FamilySlot { int slot; SynthEngine* eng; };
    juce::Array<FamilySlot> familySlots;
    familySlots.ensureStorageAllocated(MaxSlots);

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;
        if (kFamilyIds.contains(enginePtrs[i]->getEngineId()))
            familySlots.add({ i, enginePtrs[i] });
    }

    if (familySlots.size() < 2)
        return; // nothing to bleed

    // Read macro values from cached parameter pointers — safe on audio thread
    const float communeAmt = cachedParams.ohmCommune ? cachedParams.ohmCommune->load() : 0.f;
    const float bondAmt    = cachedParams.obblBond   ? cachedParams.obblBond->load()   : 0.f;
    const float dramaAmt   = cachedParams.oleDrama   ? cachedParams.oleDrama->load()   : 0.f;

    // For each family engine, send bleed coupling to all sibling family engines
    for (const auto& src : familySlots)
    {
        const float srcSample = src.eng->getSampleForCoupling(0, 0);
        const juce::String srcId = src.eng->getEngineId();

        for (const auto& dst : familySlots)
        {
            if (dst.slot == src.slot)
                continue;

            // OHM COMMUNE macro → LFOToPitch to siblings
            if (srcId == "Ohm" && communeAmt > 0.001f)
            {
                const float bleedBuf = srcSample * communeAmt;
                dst.eng->applyCouplingInput(CouplingType::LFOToPitch,
                                            communeAmt, &bleedBuf, 1);
            }

            // OBBLIGATO BOND macro → AmpToFilter to siblings
            if (srcId == "Obbligato" && bondAmt > 0.001f)
            {
                dst.eng->applyCouplingInput(CouplingType::AmpToFilter,
                                            bondAmt, nullptr, 1);
            }

            // OLE DRAMA macro → EnvToMorph to siblings
            if (srcId == "Ole" && dramaAmt > 0.001f)
            {
                dst.eng->applyCouplingInput(CouplingType::EnvToMorph,
                                            dramaAmt, nullptr, 1);
            }
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
    newEngine->prepareSilenceGate(currentSampleRate, currentBlockSize,
                                  silenceGateHoldMs(newEngine->getEngineId()));

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

    // SRO: Clear profiler and auditor for this slot
    engineProfilers[slot].reset();
    sroAuditor.clearSlot(slot);

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

            // Resolve OddfeliX (Snap) legacy param aliases before lookup.
            // Ghost params (removed in the percussive redesign) return an empty
            // string — skip them. Renamed params map to their canonical ID.
            if (fullId.startsWith("snap_"))
            {
                fullId = xomnibus::resolveSnapParamAlias(fullId);
                if (fullId.isEmpty())
                    continue;  // param was removed — skip silently
            }

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
