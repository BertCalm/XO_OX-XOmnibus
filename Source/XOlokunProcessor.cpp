#include "XOlokunProcessor.h"
// XOlokunEditor.h is intentionally NOT included here. createEditor() is
// implemented in Source/UI/XOlokunEditor.cpp to keep the UI include chain
// (ExportDialog → XPNVelocityCurves, CouplingVisualizer → GalleryColors, etc.)
// out of this translation unit and avoid circular include complications.
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
#include "Engines/OpenSky/OpenSkyEngine.h"
#include "Engines/Ostinato/OstinatoEngine.h"
#include "Engines/OceanDeep/OceandeepEngine.h"
#include "Engines/Ouie/OuieEngine.h"
#include "Engines/Obrix/ObrixEngine.h"
#include "Engines/Orbweave/OrbweaveEngine.h"
#include "Engines/Overtone/OvertoneEngine.h"
#include "Engines/Organism/OrganismEngine.h"
#include "Engines/Oxbow/OxbowEngine.h"
#include "Engines/Oware/OwareEngine.h"
#include "Engines/Opera/OperaAdapter.h"
#include "Engines/Offering/OfferingEngine.h"
#include "Engines/Oto/OtoEngine.h"
#include "Engines/Octave/OctaveEngine.h"
#include "Engines/Oleg/OlegEngine.h"
#include "Engines/Otis/OtisEngine.h"
#include "Engines/Oven/OvenEngine.h"
#include "Engines/Ochre/OchreEngine.h"
#include "Engines/Obelisk/ObeliskEngine.h"
#include "Engines/Opaline/OpalineEngine.h"
// CELLAR Quad Collection
#include "Engines/Ogre/OgreEngine.h"
#include "Engines/Olate/OlateEngine.h"
#include "Engines/Oaken/OakenEngine.h"
#include "Engines/Omega/OmegaEngine.h"
// GARDEN Quad Collection
#include "Engines/Orchard/OrchardEngine.h"
#include "Engines/Overgrow/OvergrowEngine.h"
#include "Engines/Osier/OsierEngine.h"
#include "Engines/Oxalis/OxalisEngine.h"
// BROTH Quad Collection
#include "Engines/Overwash/OverwashEngine.h"
#include "Engines/Overworn/OverwornEngine.h"
#include "Engines/Overflow/OverflowEngine.h"
#include "Engines/Overcast/OvercastEngine.h"
// FUSION Quad Collection
#include "Engines/Oasis/OasisEngine.h"
#include "Engines/Oddfellow/OddfellowEngine.h"
#include "Engines/Onkolo/OnkoloEngine.h"
#include "Engines/Opcode/OpcodeEngine.h"
#include "Engines/Osmosis/OsmosisEngine.h"
#include "Engines/Oxytocin/OxytocinAdapter.h"
// OUTLOOK — panoramic visionary synth
#include "Engines/Outlook/OutlookEngine.h"

// Register engines with their canonical IDs (matching getEngineId() return values).
// These MUST match the string returned by each engine's getEngineId().
// Legacy names ("Snap", "Morph", etc.) are resolved by resolveEngineAlias() in PresetManager.h.
static bool registered_OddfeliX = xolokun::EngineRegistry::instance().registerEngine(
    "OddfeliX", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::SnapEngine>();
    });
static bool registered_OddOscar = xolokun::EngineRegistry::instance().registerEngine(
    "OddOscar", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::MorphEngine>();
    });
static bool registered_Overdub = xolokun::EngineRegistry::instance().registerEngine(
    "Overdub", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::DubEngine>();
    });
static bool registered_Odyssey = xolokun::EngineRegistry::instance().registerEngine(
    "Odyssey", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::DriftEngine>();
    });
static bool registered_Oblong = xolokun::EngineRegistry::instance().registerEngine(
    "Oblong", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::BobEngine>();
    });
static bool registered_Obese = xolokun::EngineRegistry::instance().registerEngine(
    "Obese", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::FatEngine>();
    });
static bool registered_Onset = xolokun::EngineRegistry::instance().registerEngine(
    "Onset", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OnsetEngine>();
    });
static bool registered_Overworld = xolokun::EngineRegistry::instance().registerEngine(
    "Overworld", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OverworldEngine>();
    });
static bool registered_Opal = xolokun::EngineRegistry::instance().registerEngine(
    "Opal", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OpalEngine>();
    });
static bool registered_Overbite = xolokun::EngineRegistry::instance().registerEngine(
    "Overbite", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::BiteEngine>();
    });
static bool registered_Organon = xolokun::EngineRegistry::instance().registerEngine(
    "Organon", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrganonEngine>();
    });
static bool registered_Ocelot = xolokun::EngineRegistry::instance().registerEngine(
    "Ocelot", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xocelot::OcelotEngine>();
    });
static bool registered_Ouroboros = xolokun::EngineRegistry::instance().registerEngine(
    "Ouroboros", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OuroborosEngine>();
    });
static bool registered_Obsidian = xolokun::EngineRegistry::instance().registerEngine(
    "Obsidian", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObsidianEngine>();
    });
static bool registered_Origami = xolokun::EngineRegistry::instance().registerEngine(
    "Origami", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrigamiEngine>();
    });
static bool registered_Oracle = xolokun::EngineRegistry::instance().registerEngine(
    "Oracle", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OracleEngine>();
    });
static bool registered_Obscura = xolokun::EngineRegistry::instance().registerEngine(
    "Obscura", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObscuraEngine>();
    });
static bool registered_Oceanic = xolokun::EngineRegistry::instance().registerEngine(
    "Oceanic", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OceanicEngine>();
    });
static bool registered_Optic = xolokun::EngineRegistry::instance().registerEngine(
    "Optic", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OpticEngine>();
    });
static bool registered_Oblique = xolokun::EngineRegistry::instance().registerEngine(
    "Oblique", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObliqueEngine>();
    });
static bool registered_Orbital = xolokun::EngineRegistry::instance().registerEngine(
    "Orbital", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrbitalEngine>();
    });
static bool registered_Osprey = xolokun::EngineRegistry::instance().registerEngine(
    "Osprey", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OspreyEngine>();
    });
static bool registered_Osteria = xolokun::EngineRegistry::instance().registerEngine(
    "Osteria", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OsteriaEngine>();
    });
static bool registered_Owlfish = xolokun::EngineRegistry::instance().registerEngine(
    "Owlfish", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xowlfish::OwlfishEngine>();
    });
static bool registered_Ohm = xolokun::EngineRegistry::instance().registerEngine(
    "Ohm", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OhmEngine>();
    });
static bool registered_Orphica = xolokun::EngineRegistry::instance().registerEngine(
    "Orphica", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrphicaEngine>();
    });
static bool registered_Obbligato = xolokun::EngineRegistry::instance().registerEngine(
    "Obbligato", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObbligatoEngine>();
    });
static bool registered_Ottoni = xolokun::EngineRegistry::instance().registerEngine(
    "Ottoni", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OttoniEngine>();
    });
static bool registered_Ole = xolokun::EngineRegistry::instance().registerEngine(
    "Ole", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OleEngine>();
    });
static bool registered_Overlap = xolokun::EngineRegistry::instance().registerEngine(
    "Overlap", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::XOverlapEngine>();
    });
static bool registered_Outwit = xolokun::EngineRegistry::instance().registerEngine(
    "Outwit", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::XOutwitEngine>();
    });
static bool registered_Ombre = xolokun::EngineRegistry::instance().registerEngine(
    "Ombre", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OmbreEngine>();
    });
static bool registered_Orca = xolokun::EngineRegistry::instance().registerEngine(
    "Orca", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrcaEngine>();
    });
static bool registered_Octopus = xolokun::EngineRegistry::instance().registerEngine(
    "Octopus", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OctopusEngine>();
    });
// V1 Concept Engines — OPENSKY
static bool registered_OpenSky = xolokun::EngineRegistry::instance().registerEngine(
    "OpenSky", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OpenSkyEngine>();
    });
// V1 Concept Engines — OSTINATO
static bool registered_Ostinato = xolokun::EngineRegistry::instance().registerEngine(
    "Ostinato", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OstinatoEngine>();
    });
// V1 Concept Engines — OCEANDEEP
static bool registered_OceanDeep = xolokun::EngineRegistry::instance().registerEngine(
    "OceanDeep", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OceandeepEngine>();
    });
// V1 Concept Engines — OUIE
static bool registered_Ouie = xolokun::EngineRegistry::instance().registerEngine(
    "Ouie", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OuieEngine>();
    });
// Flagship — OBRIX (modular brick synthesis)
static bool registered_Obrix = xolokun::EngineRegistry::instance().registerEngine(
    "Obrix", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObrixEngine>();
    });
// Theorem Engine — ORBWEAVE (topological knot coupling)
static bool registered_Orbweave = xolokun::EngineRegistry::instance().registerEngine(
    "Orbweave", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrbweaveEngine>();
    });
// Theorem Engine — OVERTONE (continued fraction spectral synthesis)
static bool registered_Overtone = xolokun::EngineRegistry::instance().registerEngine(
    "Overtone", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OvertoneEngine>();
    });
// Theorem Engine — ORGANISM (cellular automata generative synthesis)
static bool registered_Organism = xolokun::EngineRegistry::instance().registerEngine(
    "Organism", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrganismEngine>();
    });
// Singularity Collection — OXBOW (entangled reverb synth engine)
static bool registered_Oxbow = xolokun::EngineRegistry::instance().registerEngine(
    "Oxbow", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OxbowEngine>();
    });
// OWARE — tuned percussion synthesizer (wood/metal material continuum)
static bool registered_Oware = xolokun::EngineRegistry::instance().registerEngine(
    "Oware", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OwareEngine>();
    });
// OPERA — additive-vocal Kuramoto synchronicity engine (Humpback Whale / SOFAR)
static bool registered_Opera = xolokun::EngineRegistry::instance().registerEngine(
    "Opera", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OperaAdapter>();
    });
// OFFERING — psychology-driven boom bap drum synthesis (Mantis Shrimp / Rubble Zone)
static bool registered_Offering = xolokun::EngineRegistry::instance().registerEngine(
    "Offering", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OfferingEngine>();
    });
// Chef Quad Collection — OTO (tape & circuit-bent heritage)
static bool registered_Oto = xolokun::EngineRegistry::instance().registerEngine(
    "Oto", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OtoEngine>();
    });
// Chef Quad Collection — OCTAVE (harmonic interval synthesis)
static bool registered_Octave = xolokun::EngineRegistry::instance().registerEngine(
    "Octave", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OctaveEngine>();
    });
// Chef Quad Collection — OLEG (folk/modal string synthesis)
static bool registered_Oleg = xolokun::EngineRegistry::instance().registerEngine(
    "Oleg", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OlegEngine>();
    });
// Chef Quad Collection — OTIS (soul/funk synthesis)
static bool registered_Otis = xolokun::EngineRegistry::instance().registerEngine(
    "Otis", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OtisEngine>();
    });
// KITCHEN Quad Collection — OVEN
static bool registered_Oven = xolokun::EngineRegistry::instance().registerEngine(
    "Oven", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OvenEngine>();
    });
// KITCHEN Quad Collection — OCHRE
static bool registered_Ochre = xolokun::EngineRegistry::instance().registerEngine(
    "Ochre", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OchreEngine>();
    });
// KITCHEN Quad Collection — OBELISK
static bool registered_Obelisk = xolokun::EngineRegistry::instance().registerEngine(
    "Obelisk", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObeliskEngine>();
    });
// KITCHEN Quad Collection — OPALINE
static bool registered_Opaline = xolokun::EngineRegistry::instance().registerEngine(
    "Opaline", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OpalineEngine>();
    });
// CELLAR Quad Collection — OGRE
static bool registered_Ogre = xolokun::EngineRegistry::instance().registerEngine(
    "Ogre", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OgreEngine>();
    });
// CELLAR Quad Collection — OLATE
static bool registered_Olate = xolokun::EngineRegistry::instance().registerEngine(
    "Olate", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OlateEngine>();
    });
// CELLAR Quad Collection — OAKEN
static bool registered_Oaken = xolokun::EngineRegistry::instance().registerEngine(
    "Oaken", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OakenEngine>();
    });
// CELLAR Quad Collection — OMEGA
static bool registered_Omega = xolokun::EngineRegistry::instance().registerEngine(
    "Omega", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OmegaEngine>();
    });
// GARDEN Quad Collection — ORCHARD
static bool registered_Orchard = xolokun::EngineRegistry::instance().registerEngine(
    "Orchard", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OrchardEngine>();
    });
// GARDEN Quad Collection — OVERGROW
static bool registered_Overgrow = xolokun::EngineRegistry::instance().registerEngine(
    "Overgrow", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OvergrowEngine>();
    });
// GARDEN Quad Collection — OSIER
static bool registered_Osier = xolokun::EngineRegistry::instance().registerEngine(
    "Osier", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OsierEngine>();
    });
// GARDEN Quad Collection — OXALIS
static bool registered_Oxalis = xolokun::EngineRegistry::instance().registerEngine(
    "Oxalis", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OxalisEngine>();
    });
// BROTH Quad Collection — OVERWASH
static bool registered_Overwash = xolokun::EngineRegistry::instance().registerEngine(
    "Overwash", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OverwashEngine>();
    });
// BROTH Quad Collection — OVERWORN
static bool registered_Overworn = xolokun::EngineRegistry::instance().registerEngine(
    "Overworn", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OverwornEngine>();
    });
// BROTH Quad Collection — OVERFLOW
static bool registered_Overflow = xolokun::EngineRegistry::instance().registerEngine(
    "Overflow", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OverflowEngine>();
    });
// BROTH Quad Collection — OVERCAST
static bool registered_Overcast = xolokun::EngineRegistry::instance().registerEngine(
    "Overcast", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OvercastEngine>();
    });
// FUSION Quad Collection — OASIS
static bool registered_Oasis = xolokun::EngineRegistry::instance().registerEngine(
    "Oasis", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OasisEngine>();
    });
// FUSION Quad Collection — ODDFELLOW
static bool registered_Oddfellow = xolokun::EngineRegistry::instance().registerEngine(
    "Oddfellow", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OddfellowEngine>();
    });
// FUSION Quad Collection — ONKOLO
static bool registered_Onkolo = xolokun::EngineRegistry::instance().registerEngine(
    "Onkolo", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OnkoloEngine>();
    });
// FUSION Quad Collection — OPCODE
static bool registered_Opcode = xolokun::EngineRegistry::instance().registerEngine(
    "Opcode", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OpcodeEngine>();
    });
// Membrane Collection — OSMOSIS (engine #47, external audio membrane)
static bool registered_Osmosis = xolokun::EngineRegistry::instance().registerEngine(
    "Osmosis", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OsmosisEngine>();
    });
// OXYTOCIN — circuit-modeling love-triangle synth, Engine #48 (Synapse Violet)
static bool registered_Oxytocin = xolokun::EngineRegistry::instance().registerEngine(
    "Oxytocin", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OxytocinAdapter>();
    });
// OUTLOOK — panoramic visionary synth (Albatross / Surface Soarer)
static bool registered_Outlook = xolokun::EngineRegistry::instance().registerEngine(
    "Outlook", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OutlookEngine>();
    });

namespace xolokun {

XOlokunProcessor::XOlokunProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), false)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "XOlokunParams", createParameterLayout()),
      couplingPresetManager(apvts, [this](int slot) -> juce::String {
          auto* eng = getEngine(slot);
          return eng ? eng->getEngineId() : juce::String{};
      })
{
    cacheParameterPointers();

    // Wire MIDI learn manager to the APVTS and install default CC mappings.
    midiLearnManager.setAPVTS(&apvts);
    midiLearnManager.loadDefaultMappings();

    // Scan default coupling presets directory on startup.
    couplingPresetManager.scanDirectory(CouplingPresetManager::getDefaultDirectory());
}

XOlokunProcessor::~XOlokunProcessor() = default;

bool XOlokunProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    const auto& inputSet = layouts.getMainInputChannelSet();
    if (inputSet != juce::AudioChannelSet::disabled()
        && inputSet != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void XOlokunProcessor::cacheParameterPointers()
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

    // Coupling performance overlay — cache all 20 params for audio-thread access
    for (int r = 0; r < CouplingCrossfader::MaxRouteSlots; ++r)
    {
        const auto prefix = "cp_r" + juce::String(r + 1) + "_";
        cachedParams.cpRoutes[static_cast<size_t>(r)].active = apvts.getRawParameterValue(prefix + "active");
        cachedParams.cpRoutes[static_cast<size_t>(r)].type   = apvts.getRawParameterValue(prefix + "type");
        cachedParams.cpRoutes[static_cast<size_t>(r)].amount = apvts.getRawParameterValue(prefix + "amount");
        cachedParams.cpRoutes[static_cast<size_t>(r)].source = apvts.getRawParameterValue(prefix + "source");
        cachedParams.cpRoutes[static_cast<size_t>(r)].target = apvts.getRawParameterValue(prefix + "target");

        // B3: Assert that all parameter pointers resolved — catches typos in param IDs.
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].active != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].type   != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].amount != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].source != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].target != nullptr);
    }

    // MPE params
    cachedParams.mpeEnabled        = apvts.getRawParameterValue("mpe_enabled");
    cachedParams.mpeZone           = apvts.getRawParameterValue("mpe_zone");
    cachedParams.mpePitchBendRange = apvts.getRawParameterValue("mpe_pitchBendRange");
    cachedParams.mpePressureTarget = apvts.getRawParameterValue("mpe_pressureTarget");
    cachedParams.mpeSlideTarget    = apvts.getRawParameterValue("mpe_slideTarget");
}

juce::AudioProcessorValueTreeState::ParameterLayout
    XOlokunProcessor::createParameterLayout()
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
    // V1 Concept Engines
    OstinatoEngine::addParameters(params);
    OpenSkyEngine::addParameters(params);
    OceandeepEngine::addParameters(params);
    OuieEngine::addParameters(params);
    // Flagship
    ObrixEngine::addParameters(params);
    // V2 Theorem Engines
    OrbweaveEngine::addParameters(params);
    OvertoneEngine::addParameters(params);
    OrganismEngine::addParameters(params);
    // Singularity Engines
    OxbowEngine::addParameters(params);
    OwareEngine::addParameters(params);
    // Kuramoto Vocal Synthesis
    opera::OperaEngine::addParameters(params);
    // Psychology-Driven Drum Synthesis
    OfferingEngine::addParameters(params);
    // Chef Quad Collection
    OtoEngine::addParameters(params);
    OctaveEngine::addParameters(params);
    OlegEngine::addParameters(params);
    OtisEngine::addParameters(params);
    // KITCHEN Quad Collection
    OvenEngine::addParameters(params);
    OchreEngine::addParameters(params);
    ObeliskEngine::addParameters(params);
    OpalineEngine::addParameters(params);
    // CELLAR Quad Collection
    OgreEngine::addParameters(params);
    OlateEngine::addParameters(params);
    OakenEngine::addParameters(params);
    OmegaEngine::addParameters(params);
    // GARDEN Quad Collection
    OrchardEngine::addParameters(params);
    OvergrowEngine::addParameters(params);
    OsierEngine::addParameters(params);
    OxalisEngine::addParameters(params);
    // BROTH Quad Collection
    OverwashEngine::addParameters(params);
    OverwornEngine::addParameters(params);
    OverflowEngine::addParameters(params);
    OvercastEngine::addParameters(params);
    // FUSION Quad Collection
    OasisEngine::addParameters(params);
    OddfellowEngine::addParameters(params);
    OnkoloEngine::addParameters(params);
    OpcodeEngine::addParameters(params);
    // OXYTOCIN — circuit-modeling love-triangle synth (Engine #48)
    OxytocinAdapter::addParameters(params);

    // ── Coupling Performance Overlay ──────────────────────────────────────────
    // 4 route slots × 5 params = 20 new APVTS parameters.
    // These are ephemeral live-performance controls that overlay preset coupling.
    // See Docs/specs/coupling_performance_spec.md §2.2.
    {
        // CouplingType enum labels (0–14) — must match CouplingType order in SynthEngine.h
        const juce::StringArray couplingTypeLabels {
            "AmpToFilter",       // 0
            "AmpToPitch",        // 1
            "LFOToPitch",        // 2
            "EnvToMorph",        // 3
            "AudioToFM",         // 4
            "AudioToRing",       // 5
            "FilterToFilter",    // 6
            "AmpToChoke",        // 7
            "RhythmToBlend",     // 8
            "EnvToDecay",        // 9
            "PitchToPitch",      // 10
            "AudioToWavetable",  // 11
            "AudioToBuffer",     // 12
            "KnotTopology",      // 13
            "TriangularCoupling" // 14 — XOxytocin love-triangle state transfer
        };

        const juce::StringArray slotLabels { "Slot 1", "Slot 2", "Slot 3", "Slot 4" };

        // Default target for each route: r1→1, r2→1, r3→2, r4→3
        const int defaultTargets[4] = { 1, 1, 2, 3 };

        for (int r = 0; r < 4; ++r)
        {
            const auto prefix = "cp_r" + juce::String(r + 1) + "_";

            // Active (bool, default off)
            params.push_back(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID(prefix + "active", 1),
                "CP Route " + juce::String(r + 1) + " Active",
                false));

            // Type (int choice 0-13, default 0 = AmpToFilter)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID(prefix + "type", 1),
                "CP Route " + juce::String(r + 1) + " Type",
                couplingTypeLabels, 0));

            // Amount (float, bipolar -1.0 to 1.0, default 0.0)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID(prefix + "amount", 1),
                "CP Route " + juce::String(r + 1) + " Amount",
                juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

            // Source (int choice 0-3, default 0 = Slot 1)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID(prefix + "source", 1),
                "CP Route " + juce::String(r + 1) + " Source",
                slotLabels, 0));

            // Target (int choice 0-3, default varies per route)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID(prefix + "target", 1),
                "CP Route " + juce::String(r + 1) + " Target",
                slotLabels, defaultTargets[r]));
        }
    }

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

    // Stage 5.6: fXFormant (Membrane Collection — Formant Filter)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_formantShift", 1), "Formant Shift",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_formantVowel", 1), "Formant Vowel",
        juce::NormalisableRange<float>(0.0f, 4.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_formantQ", 1), "Formant Resonance",
        juce::NormalisableRange<float>(0.5f, 20.0f, 0.0f, 0.4f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_formantMix", 1), "Formant Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 5.7: fXBreath (Membrane Collection — Breath Texture)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_breathAmount", 1), "Breath Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_breathTilt", 1), "Breath Tilt",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_breathSens", 1), "Breath Sensitivity",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_breathMix", 1), "Breath Mix",
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

    // MPE (MIDI Polyphonic Expression) — DAW-automatable per-project settings.
    // Zone layout, pitch-bend range, and expression routing targets are
    // exposed as APVTS parameters so hosts can save/recall them with the project.
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("mpe_enabled", 1), "MPE Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mpe_zone", 1), "MPE Zone",
        juce::StringArray{ "Off", "Lower", "Upper", "Both" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mpe_pitchBendRange", 1), "MPE Pitch Bend Range (semitones)",
        juce::NormalisableRange<float>(1.0f, 96.0f, 1.0f), 48.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mpe_pressureTarget", 1), "MPE Pressure Target",
        juce::StringArray{ "Filter Cutoff", "Volume", "Wavetable", "FX Send", "Macro 1 (CHARACTER)", "Macro 2 (MOVEMENT)" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mpe_slideTarget", 1), "MPE Slide Target",
        juce::StringArray{ "Filter Cutoff", "Volume", "Wavetable", "FX Send", "Macro 1 (CHARACTER)", "Macro 2 (MOVEMENT)" }, 0));

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
    if (engineId == "Onset"     || engineId == "Overbite"  || engineId == "OddfeliX"
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

void XOlokunProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Reset the PlaySurface MIDI collector to the new sample rate so
    // its timestamp conversion is accurate after host restart / rate change.
    playSurfaceMidiCollector.reset(sampleRate);

    couplingMatrix.prepare(samplesPerBlock);
    couplingCrossfader.prepare(sampleRate, samplesPerBlock);

    // Pre-allocate the merged route vector so the audio thread never allocates.
    // MaxRoutes is the upper bound; reserve once, reuse every block via clear + assign.
    mergedRoutePtr = std::make_shared<std::vector<MegaCouplingMatrix::CouplingRoute>>();
    mergedRoutePtr->reserve(static_cast<size_t>(MegaCouplingMatrix::MaxRoutes));

    chordMachine.prepare(sampleRate, samplesPerBlock);
    masterFX.prepare(sampleRate, samplesPerBlock, apvts);

    // SRO: Prepare profilers and auditor
    for (auto& prof : engineProfilers)
        prof.prepare(sampleRate, samplesPerBlock);
    sroAuditor.prepare(sampleRate, samplesPerBlock);

    for (auto& buf : engineBuffers)
        buf.setSize(2, samplesPerBlock);

    crossfadeBuffer.setSize(2, samplesPerBlock);
    externalInputBuffer.setSize(2, samplesPerBlock);

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

void XOlokunProcessor::releaseResources()
{
    for (int i = 0; i < MaxSlots; ++i)
    {
        auto eng = std::atomic_load(&engines[i]);
        if (eng)
            eng->releaseResources();
    }
    masterFX.reset();
}

void XOlokunProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Capture external audio input before clearing the buffer.
    // externalInputBuffer was pre-allocated in prepareToPlay — no setSize here.
    // ORDERING CONTRACT: setExternalInput() on Osmosis must be called AFTER this
    // and BEFORE renderBlock() in the same processBlock call.
    const int totalInputChannels = getTotalNumInputChannels();
    if (totalInputChannels >= 2)
    {
        for (int ch = 0; ch < 2; ++ch)
            externalInputBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    else
    {
        externalInputBuffer.clear();
    }

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

    // Merge PlaySurface note-on/off events queued from the message thread.
    // removeNextBlockOfMessages() is thread-safe; it drains the lock-free queue
    // and appends the messages into `midi` so the rest of the pipeline sees them
    // exactly like host-generated MIDI events.
    playSurfaceMidiCollector.removeNextBlockOfMessages(midi, numSamples);

    // Process MIDI learn CC → parameter routing (audio thread safe)
    midiLearnManager.processMidi(midi);

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

    // Sync MPE manager from cached APVTS parameters (no hash lookups)
    if (cachedParams.mpeEnabled)
    {
        mpeManager.setMPEEnabled(cachedParams.mpeEnabled->load() >= 0.5f);
        mpeManager.setZoneLayout(static_cast<MPEZoneLayout>(
            static_cast<int>(cachedParams.mpeZone->load())));
        mpeManager.setPitchBendRange(
            static_cast<int>(cachedParams.mpePitchBendRange->load()));
        mpeManager.setPressureTarget(static_cast<MPEManager::ExpressionTarget>(
            static_cast<int>(cachedParams.mpePressureTarget->load())));
        mpeManager.setSlideTarget(static_cast<MPEManager::ExpressionTarget>(
            static_cast<int>(cachedParams.mpeSlideTarget->load())));
    }

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

    // Feed external audio to Osmosis if loaded in any slot.
    // Uses virtual isAnalysisEngine() instead of dynamic_cast to avoid RTTI on audio thread.
    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        if (enginePtrs[slot] && enginePtrs[slot]->isAnalysisEngine())
        {
            static_cast<OsmosisEngine*>(enginePtrs[slot])->setExternalInput(
                externalInputBuffer.getReadPointer(0),
                externalInputBuffer.getReadPointer(1),
                numSamples);
        }
    }

    // Render each active engine into its own buffer using slot-specific MIDI
    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;

        // SRO: Wake silence gate on note-on events (BEFORE bypass check)
        // Field Map: push note events into the lock-free queue for UI-thread drain.
        {
            bool gateWoken = false;
            for (const auto metadata : slotMidi[i])
            {
                if (metadata.getMessage().isNoteOn())
                {
                    if (!gateWoken) { enginePtrs[i]->wakeSilenceGate(); gateWoken = true; }
                    // Push to Field Map queue (lock-free, drops if full — no block)
                    const auto& msg = metadata.getMessage();
                    pushNoteEvent(msg.getNoteNumber(),
                                  msg.getFloatVelocity(),
                                  i);
                }
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

    // -------------------------------------------------------------------
    // Performance coupling overlay — merge APVTS performance routes into
    // the baseline route list. Performance routes can override baseline
    // routes (same source->dest pair) or append new ones.
    //
    // Audio-thread safe: reads only from cached atomic pointers. Uses
    // mergedRoutePtr (a pre-allocated shared_ptr<vector> reserved to
    // MaxRoutes capacity in prepareToPlay). Each block we clear + assign
    // into it — no heap allocation because capacity >= MaxRoutes.
    // -------------------------------------------------------------------
    {
        bool anyPerfRouteActive = false;

        // Quick scan — avoid the copy entirely when no perf routes are active.
        for (int i = 0; i < CouplingCrossfader::MaxRouteSlots; ++i)
        {
            auto& cp = cachedParams.cpRoutes[static_cast<size_t>(i)];
            if (cp.active && cp.active->load() > 0.5f)
            {
                anyPerfRouteActive = true;
                break;
            }
        }

        if (anyPerfRouteActive && couplingRoutes)
        {
            // Copy baseline routes into pre-allocated storage — no heap allocation.
            // mergedRoutePtr's vector was reserved to MaxRoutes in prepareToPlay,
            // so clear + assign won't reallocate.
            auto& merged = *mergedRoutePtr;
            merged.clear();
            merged.assign(couplingRoutes->begin(), couplingRoutes->end());

            for (int i = 0; i < CouplingCrossfader::MaxRouteSlots; ++i)
            {
                auto& cp = cachedParams.cpRoutes[static_cast<size_t>(i)];

                // B2: Null guard — if any param pointer failed to resolve,
                // skip this slot entirely. jassertfalse fires in debug builds
                // to catch the misconfiguration early.
                if (!cp.type || !cp.amount || !cp.source || !cp.target)
                {
                    jassertfalse;
                    continue;
                }

                if (!cp.active || cp.active->load() <= 0.5f)
                {
                    // Inactive slot — reset crossfader state so the next
                    // activation starts clean (no stale previousType).
                    couplingCrossfader.resetSlot(i);
                    continue;
                }

                // B4: Clamp raw enum value to valid CouplingType range to prevent
                // undefined behavior from out-of-range parameter values.
                const int rawType = juce::jlimit(0,
                    static_cast<int>(CouplingType::KnotTopology),
                    juce::roundToInt(cp.type->load()));
                const auto perfType = static_cast<CouplingType>(rawType);

                const float perfAmount = cp.amount->load();
                const int perfSource   = static_cast<int>(cp.source->load());
                const int perfTarget   = static_cast<int>(cp.target->load());

                // Bounds-check slot indices to prevent OOB
                if (perfSource < 0 || perfSource >= MaxSlots
                 || perfTarget < 0 || perfTarget >= MaxSlots
                 || perfSource == perfTarget)
                    continue;

                // Detect coupling type changes and trigger crossfade if needed.
                // updateRouteType() returns true when a crossfade is in progress
                // (audio-rate types like FM/Ring/Wavetable/Buffer/Knot need smooth
                // transitions). The crossfade state lives inside couplingCrossfader
                // and is consumed by MegaCouplingMatrix during processBlock via
                // the route's type field — the crossfader manages which type(s)
                // to evaluate, so we always pass the current (target) type here.
                couplingCrossfader.updateRouteType(i, perfType);

                // Build the route struct
                MegaCouplingMatrix::CouplingRoute perfRoute;
                perfRoute.sourceSlot  = perfSource;
                perfRoute.destSlot    = perfTarget;
                perfRoute.type        = perfType;
                perfRoute.amount      = perfAmount;
                perfRoute.isNormalled = false;
                perfRoute.active      = true;

                // Override or append: if a baseline route targets the same
                // source->dest pair, the performance route wins.
                bool overridden = false;
                for (auto& baseRoute : merged)
                {
                    if (baseRoute.sourceSlot == perfSource
                     && baseRoute.destSlot   == perfTarget)
                    {
                        baseRoute.type   = perfRoute.type;
                        baseRoute.amount = perfRoute.amount;
                        baseRoute.active = true;
                        overridden = true;
                        break;
                    }
                }

                if (!overridden)
                {
                    // Append — guard against exceeding MaxRoutes
                    if (static_cast<int>(merged.size()) < MegaCouplingMatrix::MaxRoutes)
                        merged.push_back(perfRoute);
                }
            }

            couplingRoutes = mergedRoutePtr;
        }
        else if (!anyPerfRouteActive)
        {
            // No perf routes active — reset all crossfader slots so stale
            // state doesn't linger across preset loads.
            couplingCrossfader.resetAll();
        }
    }

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

    // Process crossfade tails for outgoing engines.
    // Snapshot outgoing engine shared_ptrs under the mutex to prevent a race
    // with loadEngine/unloadEngine (which write crossfades[] on the message thread).
    // DSP rendering happens outside the lock so the message thread is never
    // blocked for a full audio block duration.
    std::array<std::shared_ptr<SynthEngine>, MaxSlots> outgoingSnapshot;
    std::array<float, MaxSlots> fadeGainSnapshot  = {};
    std::array<int,   MaxSlots> fadeSamplesSnapshot = {};
    {
        std::scoped_lock lock(crossfadeMutex);
        for (int i = 0; i < MaxSlots; ++i)
        {
            outgoingSnapshot[i]    = crossfades[i].outgoing;
            fadeGainSnapshot[i]    = crossfades[i].fadeGain;
            fadeSamplesSnapshot[i] = crossfades[i].fadeSamplesRemaining;
        }
    }

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (fadeSamplesSnapshot[i] <= 0 || !outgoingSnapshot[i])
            continue;

        crossfadeBuffer.clear();
        juce::MidiBuffer emptyMidi;
        outgoingSnapshot[i]->renderBlock(crossfadeBuffer, emptyMidi, numSamples);

        int fadeSamples = std::min(numSamples, fadeSamplesSnapshot[i]);
        if (fadeSamples <= 0)
            continue;
        // Distribute fade over ALL remaining samples, not just this block,
        // so the crossfade takes the full 50ms regardless of block size.
        float fadeStep = fadeGainSnapshot[i] / static_cast<float>(fadeSamplesSnapshot[i]);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float gain = fadeGainSnapshot[i];
            auto* dest = buffer.getWritePointer(ch);
            auto* src = crossfadeBuffer.getReadPointer(ch);
            for (int s = 0; s < fadeSamples; ++s)
            {
                dest[s] += src[s] * gain * masterVol;
                gain -= fadeStep;
            }
        }

        // Write updated crossfade state back under the lock.
        // Another loadEngine call between the snapshot and here is benign:
        // it would have reset fadeGain=1.0 and fadeSamplesRemaining=full,
        // so our decrement would be overwritten by their reset — correct behavior.
        {
            std::scoped_lock lock(crossfadeMutex);
            auto& cf = crossfades[i];
            // Only update if outgoing pointer hasn't been replaced by a new swap.
            // If loadEngine fired between snapshot and now, cf.outgoing will differ
            // from outgoingSnapshot[i] — leave the new crossfade state untouched.
            if (cf.outgoing == outgoingSnapshot[i])
            {
                cf.fadeGain -= fadeStep * static_cast<float>(fadeSamples);
                cf.fadeSamplesRemaining -= fadeSamples;

                if (cf.fadeSamplesRemaining <= 0)
                {
                    cf.outgoing.reset();
                    cf.fadeGain = 0.0f;
                }
            }
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

void XOlokunProcessor::processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs)
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
                const float bondBuf = srcSample * bondAmt;
                dst.eng->applyCouplingInput(CouplingType::AmpToFilter,
                                            bondAmt, &bondBuf, 1);
            }

            // OLE DRAMA macro → EnvToMorph to siblings
            if (srcId == "Ole" && dramaAmt > 0.001f)
            {
                const float dramaBuf = srcSample * dramaAmt;
                dst.eng->applyCouplingInput(CouplingType::EnvToMorph,
                                            dramaAmt, &dramaBuf, 1);
            }
        }
    }
}

void XOlokunProcessor::loadEngine(int slot, const std::string& engineId)
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

    // Wake the silence gate so the new engine renders its first block immediately.
    // Without this, a freshly-constructed engine's gate is in the bypassed state —
    // any note held across a mid-note swap would produce a one-block DSP gap
    // because processBlock would skip the new engine before its first note-on.
    newEngine->wakeSilenceGate();

    // Move the old engine to crossfade-out state
    auto oldEngine = std::atomic_load(&engines[slot]);
    if (oldEngine)
    {
        std::scoped_lock lock(crossfadeMutex);
        crossfades[slot].outgoing = oldEngine;
        crossfades[slot].fadeGain = 1.0f;
        crossfades[slot].fadeSamplesRemaining =
            static_cast<int>(currentSampleRate * CrossfadeMs * 0.001);
    }

    // Atomic swap — audio thread sees the new engine on next block
    auto shared = std::shared_ptr<SynthEngine>(std::move(newEngine));
    std::atomic_store(&engines[slot], shared);

    // Suspend coupling routes that are incompatible with the new engine.
    // AudioToBuffer routes require OpalEngine as the destination — any other
    // engine as dest will silently fail the dynamic_cast in processAudioRoute().
    // This marks such routes inactive so the UI can surface them as orphaned
    // and the user can reconnect them. Safe to call after the atomic swap.
    couplingMatrix.notifyCouplingMatrixOfSwap(slot, engineId);

    if (onEngineChanged)
        juce::MessageManager::callAsync([this, slot]{ if (onEngineChanged) onEngineChanged(slot); });
}

void XOlokunProcessor::unloadEngine(int slot)
{
    if (slot < 0 || slot >= MaxSlots)
        return;

    auto oldEngine = std::atomic_load(&engines[slot]);
    if (oldEngine)
    {
        std::scoped_lock lock(crossfadeMutex);
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

SynthEngine* XOlokunProcessor::getEngine(int slot) const
{
    if (slot >= 0 && slot < MaxSlots)
    {
        auto eng = std::atomic_load(&engines[slot]);
        return eng.get();
    }
    return nullptr;
}

// createEditor() is implemented in Source/UI/XOlokunEditor.cpp

void XOlokunProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml)
    {
        // FIX 4 — State schema version (bump when format changes).
        // v1 = legacy (no slot/coupling/CM data); v2 = slots + coupling + CM.
        xml->setAttribute("stateVersion", 2);

        // FIX 1 — Save engine slot selections so DAW session recall restores
        // the loaded engine layout.  An empty string means the slot is empty.
        for (int i = 0; i < MaxSlots; ++i)
        {
            auto eng = std::atomic_load(&engines[i]);
            juce::String slotKey = "slot" + juce::String(i) + "Engine";
            xml->setAttribute(slotKey, eng ? juce::String(eng->getEngineId()) : juce::String{});
        }

        // FIX 2 — Save baseline coupling routes (user-created routes in the
        // MegaCouplingMatrix that are NOT stored in the APVTS).
        auto routes = couplingMatrix.loadRoutes();
        if (routes && !routes->empty())
        {
            auto* routesElem = xml->createNewChildElement("BaselineCouplingRoutes");
            for (const auto& r : *routes)
            {
                auto* re = routesElem->createNewChildElement("Route");
                re->setAttribute("src",       r.sourceSlot);
                re->setAttribute("dst",       r.destSlot);
                re->setAttribute("type",      static_cast<int>(r.type));
                re->setAttribute("amount",    static_cast<double>(r.amount));
                re->setAttribute("normalled", r.isNormalled);
                re->setAttribute("active",    r.active);
            }
        }

        // FIX 3 — Save ChordMachine per-step sequencer data.  The APVTS
        // captures pattern-template selection but not custom per-step edits.
        juce::String cmStateJson = juce::JSON::toString(chordMachine.serializeState());
        xml->setAttribute("chordMachineState", cmStateJson);

        // Persist MIDI learn mappings alongside APVTS state so DAW project
        // recall restores CC → parameter assignments.
        juce::String mappingsJson = juce::JSON::toString(midiLearnManager.toJSON());
        xml->setAttribute("midiLearnMappings", mappingsJson);

        copyXmlToBinary(*xml, destData);
    }
}

void XOlokunProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

        // FIX 4 — Read schema version so we can gate new restore blocks
        // against legacy saves that predate v2 (no slot/coupling/CM data).
        const int stateVersion = xml->getIntAttribute("stateVersion", 1);

        if (stateVersion >= 2)
        {
            // FIX 1 — Restore engine slot selections.  Must run AFTER
            // apvts.replaceState() so parameters are in place when
            // attachParameters() is called inside loadEngine().
            // prepareToPlay() always follows setStateInformation and will
            // re-call eng->prepare() with the correct sample rate / block
            // size, so using the current defaults here is safe.
            for (int i = 0; i < MaxSlots; ++i)
            {
                juce::String slotKey = "slot" + juce::String(i) + "Engine";
                if (xml->hasAttribute(slotKey))
                {
                    juce::String engineId = xml->getStringAttribute(slotKey);
                    if (engineId.isNotEmpty())
                    {
                        // Resolve legacy aliases (e.g. "Snap" → "OddfeliX")
                        engineId = resolveEngineAlias(engineId);
                        if (EngineRegistry::instance().isRegistered(engineId.toStdString()))
                            loadEngine(i, engineId.toStdString());
                        // Unknown / unregistered IDs are silently skipped
                        // (slot remains empty) rather than crashing.
                    }
                }
            }

            // FIX 2 — Restore baseline coupling routes.
            couplingMatrix.clearRoutes();
            if (auto* routesElem = xml->getChildByName("BaselineCouplingRoutes"))
            {
                for (auto* re : routesElem->getChildIterator())
                {
                    MegaCouplingMatrix::CouplingRoute r;
                    r.sourceSlot  = re->getIntAttribute("src", 0);
                    r.destSlot    = re->getIntAttribute("dst", 1);
                    r.type        = static_cast<CouplingType>(re->getIntAttribute("type", 0));
                    r.amount      = static_cast<float>(re->getDoubleAttribute("amount", 0.5));
                    r.isNormalled = re->getBoolAttribute("normalled", false);
                    r.active      = re->getBoolAttribute("active", true);

                    // Bounds-check before adding to prevent corrupted saves
                    // from crashing or creating self-routing loops.
                    if (r.sourceSlot >= 0 && r.sourceSlot < MaxSlots
                     && r.destSlot   >= 0 && r.destSlot   < MaxSlots
                     && r.sourceSlot != r.destSlot)
                    {
                        couplingMatrix.addRoute(r);
                    }
                }
            }

            // FIX 3 — Restore ChordMachine per-step sequencer data.
            if (xml->hasAttribute("chordMachineState"))
            {
                juce::var cmState = juce::JSON::parse(
                    xml->getStringAttribute("chordMachineState"));
                chordMachine.restoreState(cmState);
            }
        }

        // Restore MIDI learn mappings; fall back to defaults if absent
        // (e.g. projects saved before this feature was added).
        if (xml->hasAttribute("midiLearnMappings"))
        {
            juce::var mappings = juce::JSON::parse(xml->getStringAttribute("midiLearnMappings"));
            midiLearnManager.fromJSON(mappings);
        }
        else
        {
            midiLearnManager.loadDefaultMappings();
        }
    }
}

void XOlokunProcessor::applyPreset(const PresetData& preset)
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
                fullId = xolokun::resolveSnapParamAlias(fullId);
                if (fullId.isEmpty())
                    continue;  // param was removed — skip silently
            }

            // Resolve Overbite (Bite) legacy param aliases before lookup.
            // Four schema generations coexisted; this maps Gen 2/3 names to their
            // canonical Gen 4 APVTS IDs and drops params with no equivalent.
            if (fullId.startsWith("poss_"))
            {
                fullId = xolokun::resolveBiteParamAlias(fullId);
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

} // namespace xolokun

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new xolokun::XOlokunProcessor();
}
