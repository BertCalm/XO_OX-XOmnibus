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
#include "Engines/Okeanos/OkeanosEngine.h"
#include "Engines/Oddfellow/OddfellowEngine.h"
#include "Engines/Onkolo/OnkoloEngine.h"
#include "Engines/Opcode/OpcodeEngine.h"
#include "Engines/Osmosis/OsmosisEngine.h"
#include "Engines/Oxytocin/OxytocinAdapter.h"
// OUTLOOK — panoramic visionary synth
#include "Engines/Outlook/OutlookEngine.h"
// Dual Engine Integration — OASIS + OUTFLOW
#include "Engines/Oasis/OasisEngine.h"
#include "Engines/Outflow/OutflowEngine.h"
// Cellular Automata Oscillator — OBIONT (engine #74)
#include "Engines/Obiont/ObiontEngine.h"
#include "DSP/ThreadInit.h"

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
// FUSION Quad Collection — OKEANOS (formerly Oasis, renamed to free ID for ecosystem engine)
static bool registered_Okeanos = xolokun::EngineRegistry::instance().registerEngine(
    "Okeanos", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OkeanosEngine>();
    });
// FUSION Quad Collection — OASIS (bioluminescent ecosystem engine)
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
// Dual Engine Integration — OUTFLOW (Predictive Spatial Vacuum, engine #49)
static bool registered_Outflow = xolokun::EngineRegistry::instance().registerEngine(
    "Outflow", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::OutflowEngine>();
    });
// Cellular Automata Oscillator — OBIONT (engine #74)
static bool registered_Obiont = xolokun::EngineRegistry::instance().registerEngine(
    "Obiont", []() -> std::unique_ptr<xolokun::SynthEngine> {
        return std::make_unique<xolokun::ObiontEngine>();
    });

namespace xolokun {

XOlokunProcessor::XOlokunProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), false)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "XOlokunParams", createParameterLayout()),
      macroSystem_(apvts),
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
    OkeanosEngine::addParameters(params);
    OddfellowEngine::addParameters(params);
    OnkoloEngine::addParameters(params);
    OpcodeEngine::addParameters(params);
    // OXYTOCIN — circuit-modeling love-triangle synth (Engine #48)
    OxytocinAdapter::addParameters(params);

    // W07 fix: OVERLAP, OUTWIT, OSMOSIS, OUTLOOK were using createParameterLayout()
    // instead of addParameters(), so their params were never registered in the shared
    // APVTS.  Adding them here so ParameterGrid shows knobs for these engines.
    XOverlapEngine::addParameters(params);
    XOutwitEngine::addParameters(params);
    OsmosisEngine::addParameters(params);
    OutlookEngine::addParameters(params);
    ObiontEngine::addParameters(params);
    // FUSION Quad Collection — OASIS + OUTFLOW (ecosystem engines, missing from APVTS)
    OasisEngine::addParameters(params);
    OutflowEngine::addParameters(params);

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
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f));  // skew 0.5: more resolution in subtle blend range
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
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f));  // skew 0.5: more resolution in subtle blend range
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
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f));  // skew 0.5: more resolution in subtle blend range
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_fshiftMode", 1), "Master Freq Shift Mode",
        juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_fshiftFeedback", 1), "Master Freq Shift FB",
        juce::NormalisableRange<float>(0.0f, 0.9f), 0.0f));

    // Stage 8: Multiband OTT
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_ottMix", 1), "Master OTT Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f));  // skew 0.5: more resolution in subtle blend range
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_ottDepth", 1), "Master OTT Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
    // Stage 11: Reverb (FATHOM 8-tap Hadamard FDN — 8 parameters)
    // master_reverbSize and master_reverbMix are the legacy IDs; preserved unchanged.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbSize", 1), "Master Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbMix", 1), "Master Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbPreDelay", 1), "Master Reverb Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 250.0f, 0.0f, 0.4f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbDecay", 1), "Master Reverb Decay",
        juce::NormalisableRange<float>(0.5f, 20.0f, 0.0f, 0.4f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbDamping", 1), "Master Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbDiffusion", 1), "Master Reverb Diffusion",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbMod", 1), "Master Reverb Modulation",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbWidth", 1), "Master Reverb Width",
        juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
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
        juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f), 0.0f));  // 0=Chorus 1=Flanger 2=Ensemble 3=Drift 4=Phaser
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

    // Stage 19.5: Parametric EQ (post-compressor, pre-limiter)
    // Band 1: Low shelf (20–500 Hz, ±12 dB, Q 0.5–5.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB1Freq", 1), "EQ Band 1 Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.0f, 0.35f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB1Gain", 1), "EQ Band 1 Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB1Q", 1), "EQ Band 1 Q",
        juce::NormalisableRange<float>(0.5f, 5.0f, 0.0f, 0.4f), 0.707f));
    // Band 2: Low-mid peak (100–5000 Hz, ±12 dB, Q 0.5–10.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB2Freq", 1), "EQ Band 2 Freq",
        juce::NormalisableRange<float>(100.0f, 5000.0f, 0.0f, 0.35f), 400.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB2Gain", 1), "EQ Band 2 Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB2Q", 1), "EQ Band 2 Q",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.0f, 0.4f), 1.0f));
    // Band 3: High-mid peak (500–15000 Hz, ±12 dB, Q 0.5–10.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB3Freq", 1), "EQ Band 3 Freq",
        juce::NormalisableRange<float>(500.0f, 15000.0f, 0.0f, 0.35f), 3000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB3Gain", 1), "EQ Band 3 Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB3Q", 1), "EQ Band 3 Q",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.0f, 0.4f), 1.0f));
    // Band 4: High shelf (2000–20000 Hz, ±12 dB, Q 0.5–5.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB4Freq", 1), "EQ Band 4 Freq",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.0f, 0.35f), 10000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB4Gain", 1), "EQ Band 4 Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB4Q", 1), "EQ Band 4 Q",
        juce::NormalisableRange<float>(0.5f, 5.0f, 0.0f, 0.4f), 0.707f));

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
    // Guard against non-compliant hosts or test harnesses passing invalid values.
    if (sampleRate <= 0.0) sampleRate = 44100.0;
    if (samplesPerBlock <= 0) samplesPerBlock = 512;

    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    atomicSampleRate_.store(sampleRate, std::memory_order_relaxed);
    currentBlockSize.store(samplesPerBlock, std::memory_order_relaxed);

    // Reset the PlaySurface MIDI collector to the new sample rate so
    // its timestamp conversion is accurate after host restart / rate change.
    playSurfaceMidiCollector.reset(sampleRate);

    macroSystem_.prepare(sampleRate, samplesPerBlock);
    couplingMatrix.prepare(samplesPerBlock, sampleRate);
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

    // On first launch (no saved state), load Obrix into Slot 0 so the plugin
    // opens with a working engine rather than silence.
    //
    // NOTE: This is NOT guaranteed to run only once per session. auval calls
    // prepareToPlay without calling setStateInformation, so hasRestoredState
    // will be false during AU validation. The engines[0] == nullptr guard
    // ensures loadEngine() is called at most once in that path.
    //
    // In real DAW sessions, setStateInformation() typically runs before the
    // first prepareToPlay(), setting hasRestoredState = true and bypassing
    // this block. However, this ordering is host-dependent and not part of
    // the JUCE AudioProcessor contract. (Audit P0-2 CRITICAL-2)
    if (!hasRestoredState.load(std::memory_order_acquire)
        && std::atomic_load(&engines[0]) == nullptr)
    {
        loadEngine(0, "Obrix");
    }

    // At end of prepareToPlay, clear any stale pending crossfades
    for (auto& pc : pendingCrossfades)
    {
        pc.outgoing.reset();
        pc.ready.store(false, std::memory_order_release);
    }

    // Pre-allocate familySlots so processFamilyBleed never heap-allocates
    // on the audio thread when capacity is zero on first block.
    familySlots_.ensureStorageAllocated(MaxSlots);
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
    xolokun::dsp::initAudioThreadOnce();  // ARM64 FPCR.FZ — see ThreadInit.h
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // CPU load measurement: record start time (high-res ticks)
    processBlockStartTick = juce::Time::getHighResolutionTicks();

    // Consume kill-delay-tails request — must run before any audio processing
    // so the reset takes effect before any FX contributes this block.
    if (killDelayTailsPending.load(std::memory_order_acquire))
    {
        masterFX.reset();
        killDelayTailsPending.store(false, std::memory_order_release);
    }

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

    // Propagate macro knob values (CHARACTER/MOVEMENT/COUPLING/SPACE) to their
    // per-engine target parameters before any engine renders this block.
    macroSystem_.processBlock(numSamples);

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

    // Drain pending crossfade commands posted by the message thread.
    // Lock-free SPSC: audio thread is the sole consumer; message thread is
    // the sole producer.  We consume with acquire semantics to ensure the
    // shared_ptr and scalar fields written by the message thread are visible.
    // NOTE: This drain must run BEFORE the activeCount==0 early-return so that
    // a pending crossfade command is always consumed even when the last active
    // engine has just been unloaded.
    for (int i = 0; i < MaxSlots; ++i)
    {
        auto& pending = pendingCrossfades[i];
        if (pending.ready.load(std::memory_order_acquire))
        {
            // Consume the pending command into the audio-thread-only crossfades[].
            // Move the shared_ptr to avoid a ref-count bump on the audio thread.
            crossfades[i].outgoing             = std::move(pending.outgoing);
            crossfades[i].fadeGain             = pending.fadeGain;
            crossfades[i].fadeSamplesRemaining = pending.fadeSamplesRemaining;
            crossfades[i].needsAllNotesOff     = pending.needsAllNotesOff;
            // Release the slot so the message thread may post another swap.
            pending.ready.store(false, std::memory_order_release);
        }
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
    // W12 fix: sync pattern from APVTS on every block.  applyPattern() is safe to
    // call from the audio thread — it writes to steps[].active (benign race as
    // documented in ChordMachine.h) and updates activePattern atomic.
    // We only call applyPattern when the pattern index changes to avoid clobbering
    // per-step edits the user may have made (Eno mutations, UI step overrides).
    {
        const auto newPat = static_cast<RhythmPattern>(
            static_cast<int>(cachedParams.cmSeqPattern->load()));
        if (newPat != chordMachine.getPattern())
            chordMachine.applyPattern(newPat);
    }

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

    // ── CC64 sustain pedal — fleet-wide pre-filter ────────────────────────────
    // Only ~3 engines (Ouroboros, Bite, Opal) handle CC64 internally; the other
    // ~70 engines ignore it.  This processor-level pass suppresses note-offs on
    // all per-slot MidiBuffers while CC64 >= 64, then re-injects them the moment
    // the pedal lifts.  Engines that handle CC64 themselves continue to do so —
    // they receive the CC message and the held notes land again at pedal-up just
    // as a real pianist would expect.
    //
    // Algorithm (audio-thread-only, no allocation):
    //   1. Scan raw `midi` for CC64 events; update sustainHeld_[ch].
    //      When a pedal-up event arrives, flush all pending note-offs for that
    //      channel into every slotMidi[] at sample position 0 of this block.
    //   2. Scan each slotMidi[i]: copy all non-note-off events through; for
    //      note-off events, either pass through (sustain not held) or enqueue
    //      them in sustainPendingNoteOffs_[i][ch] (sustain held).
    {
        // Step 1: update sustainHeld_ and expressionValue_ from raw input;
        //         flush pending note-offs on CC64 pedal-up.
        for (const auto metadata : midi)
        {
            const auto& msg = metadata.getMessage();

            // CC11 Expression: track per-channel value for coupling/macro use.
            // CC11 is NOT filtered — it passes through to all engine slots so
            // acoustic engines (OAKEN, OBELISK, ORCHARD, etc.) can respond to it.
            if (msg.isControllerOfType(11))
            {
                const int ch = msg.getChannel() - 1;  // 0-based
                expressionValue_[ch] = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }

            if (msg.isControllerOfType(64))
            {
                const int ch = msg.getChannel() - 1;  // 0-based
                const bool nowHeld = (msg.getControllerValue() >= 64);
                const bool wasHeld = sustainHeld_[ch];

                sustainHeld_[ch] = nowHeld;

                // Pedal released: flush deferred note-offs into every slot buffer.
                if (wasHeld && !nowHeld)
                {
                    for (int slot = 0; slot < MaxSlots; ++slot)
                    {
                        auto& pending = sustainPendingNoteOffs_[slot][ch];
                        if (!pending.any())
                            continue;
                        for (int note = 0; note < 128; ++note)
                        {
                            if (pending.test(note))
                                slotMidi[slot].addEvent(
                                    juce::MidiMessage::noteOff(ch + 1, note, (uint8_t)0), 0);
                        }
                        pending.clearAll();
                    }
                }
            }

            // CC123 All Notes Off / CC120 All Sound Off — MIDI panic.
            // Clear all sustain state so no stuck notes survive the panic message.
            if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (auto& held : sustainHeld_)
                    held = false;
                for (auto& slotPending : sustainPendingNoteOffs_)
                    for (auto& chanPending : slotPending)
                        chanPending.clearAll();
            }
        }

        // Step 2: rebuild each slotMidi[], suppressing note-offs that arrive while
        //         sustain is held on that channel.
        juce::MidiBuffer filtered;
        for (int slot = 0; slot < MaxSlots; ++slot)
        {
            filtered.clear();
            for (const auto metadata : slotMidi[slot])
            {
                const auto& msg = metadata.getMessage();
                const int samplePos = metadata.samplePosition;

                // isNoteOff() matches noteOff status bytes AND noteOn with velocity 0.
                if (msg.isNoteOff())
                {
                    const int ch   = msg.getChannel() - 1;
                    const int note = msg.getNoteNumber();
                    if (sustainHeld_[ch])
                    {
                        // Defer: remember this note needs a release later.
                        sustainPendingNoteOffs_[slot][ch].set(note);
                        continue;  // suppress the note-off for now
                    }
                    // Sustain not held: cancel any pending entry for this note
                    // (note was re-triggered and released before pedal lifted).
                    sustainPendingNoteOffs_[slot][ch].clear(note);
                }

                filtered.addEvent(msg, samplePos);
            }
            slotMidi[slot].swapWith(filtered);
        }
    }
    // ── end CC64 sustain pre-filter ───────────────────────────────────────────

    // Consume chord-fire request: inject a note-on for the current live root
    // (or MIDI C4 = 60 if no chord has been played yet) into all slot buffers.
    // This fires the current palette/voicing as a one-shot performance gesture.
    if (chordFirePending.load(std::memory_order_acquire))
    {
        chordFirePending.store(false, std::memory_order_release);
        if (chordMachine.isEnabled())
        {
            // ChordMachine already distributed notes via processBlock above.
            // A fire gesture injects an extra note-on at sample 0 into each slot
            // using the current assignment so the user hears the chord immediately.
            // Note-offs are deferred via chordFireNoteOffCountdown (~100ms hold).
            const auto& assign = chordMachine.getCurrentAssignment();
            const float vel = 0.8f;
            for (int s = 0; s < kChordSlots; ++s)
            {
                int note = assign.midiNotes[s];
                if (note < 0 || note > 127)
                    note = 60; // fallback: C4
                slotMidi[s].addEvent(
                    juce::MidiMessage::noteOn(1, note, vel), 0);
                // Store fired note for deferred note-off
                chordFireNotes[s].store(note, std::memory_order_relaxed);
            }
        }
        else
        {
            // Chord machine disabled: fire C4 on all slots as a raw trigger.
            // Note-offs deferred via chordFireNoteOffCountdown.
            for (int s = 0; s < kChordSlots; ++s)
            {
                slotMidi[s].addEvent(
                    juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
                chordFireNotes[s].store(60, std::memory_order_relaxed);
            }
        }
        // Arm the note-off countdown: hold for ~kChordHoldMs milliseconds
        chordFireNoteOffCountdown.store(
            static_cast<int>(currentSampleRate.load(std::memory_order_relaxed) * kChordHoldMs / 1000.0),
            std::memory_order_release);
    }

    // Deferred note-off for chord fire gesture.
    // Decrement the countdown each block; inject note-offs when it reaches zero.
    {
        int countdown = chordFireNoteOffCountdown.load(std::memory_order_acquire);
        if (countdown > 0)
        {
            const int newCountdown = countdown - numSamples;
            if (newCountdown <= 0)
            {
                // Countdown expired — inject note-offs for all fired chord notes
                chordFireNoteOffCountdown.store(0, std::memory_order_release);
                for (int s = 0; s < kChordSlots; ++s)
                {
                    const int note = chordFireNotes[s].load(std::memory_order_relaxed);
                    if (note > 0 && note <= 127)  // 0 = sentinel (empty); MIDI note C-1 excluded
                        slotMidi[s].addEvent(juce::MidiMessage::noteOff(1, note), 0);
                }
            }
            else
            {
                chordFireNoteOffCountdown.store(newCountdown, std::memory_order_relaxed);
            }
        }
    }

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

        // Waveform FIFO push — capture raw engine output (pre-coupling, pre-master FX)
        // for the UI oscilloscope.  No allocation; O(n) copy is fine at 512 samples.
        waveformFifos[i].push(engineBuffers[i].getReadPointer(0),
                              static_cast<size_t>(numSamples));
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

    // Decay coupling burst gain toward 1.0 each block.
    {
        int burstRemaining = couplingBurstSamplesRemaining.load(std::memory_order_relaxed);
        if (burstRemaining > 0)
        {
            const int newRemaining = std::max(0, burstRemaining - numSamples);
            couplingBurstSamplesRemaining.store(newRemaining, std::memory_order_relaxed);
            if (newRemaining == 0)
                couplingBurstGain.store(1.0f, std::memory_order_relaxed);
            else
            {
                // Linear decay from kCouplingBurstPeak to 1.0 over the burst window.
                const int totalBurstSamples = static_cast<int>(
                    currentSampleRate.load(std::memory_order_relaxed) * kCouplingBurstMs / 1000.0);
                const float progress = 1.0f - static_cast<float>(newRemaining)
                                            / static_cast<float>(std::max(1, totalBurstSamples));
                const float decayed = kCouplingBurstPeak - (kCouplingBurstPeak - 1.0f) * progress;
                couplingBurstGain.store(decayed, std::memory_order_relaxed);
            }
        }
    }

    // Apply couplingBurstGain to all active route amounts before processing.
    // This is the actual application of the burst multiplier — without this, the
    // burst gain is computed and decayed above but never heard.
    {
        const float burstGain = couplingBurstGain.load(std::memory_order_relaxed);
        if (burstGain != 1.0f && couplingRoutes)
        {
            // couplingRoutes may point to either the baseline shared_ptr or
            // mergedRoutePtr (pre-allocated, already writable this block).
            // We must not mutate the baseline shared_ptr's vector directly
            // (it is shared across threads), so only apply when couplingRoutes
            // points to our own mergedRoutePtr — otherwise copy into it first.
            if (couplingRoutes != mergedRoutePtr)
            {
                auto& merged = *mergedRoutePtr;
                merged.clear();
                merged.assign(couplingRoutes->begin(), couplingRoutes->end());
                couplingRoutes = mergedRoutePtr;
            }
            for (auto& route : *couplingRoutes)
            {
                if (route.active)
                    route.amount *= burstGain;
            }
        }
    }

    couplingMatrix.processBlock(numSamples, couplingRoutes);

    // Apply family bleed between Constellation engines (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE)
    processFamilyBleed(enginePtrs);

    // Mix all engine outputs to master — skip muted slots.
    const float masterVol = cachedParams.masterVolume->load();

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;

        // Respect per-slot mute state (written by message thread via setSlotMuted)
        if (slotMuted[i].load(std::memory_order_relaxed))
            continue;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, engineBuffers[i], ch, 0, numSamples, masterVol);
    }

    // Process crossfade tails for outgoing engines.
    // crossfades[] is now audio-thread-only — no locking required.
    // (Pending crossfade commands were already drained above, before the
    // activeCount==0 early-return, so crossfades[] is up-to-date here.)
    for (int i = 0; i < MaxSlots; ++i)
    {
        auto& cf = crossfades[i];
        if (cf.fadeSamplesRemaining <= 0 || !cf.outgoing)
            continue;

        crossfadeBuffer.clear();
        juce::MidiBuffer fadeMidi;
        // #360: On the first crossfade block, inject CC123 (All Notes Off) and
        // CC120 (All Sound Off) so any held voices in the outgoing engine release
        // cleanly instead of sustaining indefinitely (drone / pad engines).
        if (cf.needsAllNotesOff)
        {
            for (int ch = 0; ch < 16; ++ch)
            {
                fadeMidi.addEvent(juce::MidiMessage::controllerEvent(ch + 1, 123, 0), 0); // All Notes Off
                fadeMidi.addEvent(juce::MidiMessage::controllerEvent(ch + 1, 120, 0), 0); // All Sound Off
            }
            cf.needsAllNotesOff = false;
        }
        cf.outgoing->renderBlock(crossfadeBuffer, fadeMidi, numSamples);

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

    // CPU load measurement: elapsed / buffer_duration, smoothed with a leaky integrator.
    // Uses high-resolution ticks so measurements are host-independent.
    const double sr_ = currentSampleRate.load(std::memory_order_relaxed);
    if (sr_ > 0.0 && numSamples > 0)
    {
        const juce::int64 endTick     = juce::Time::getHighResolutionTicks();
        const double ticksPerSec      = static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());
        const double elapsedSec       = static_cast<double>(endTick - processBlockStartTick) / ticksPerSec;
        const double bufferDurationSec = static_cast<double>(numSamples) / sr_;
        const float  rawLoad           = static_cast<float>(elapsedSec / bufferDurationSec);
        // Leaky integrator: 90% previous + 10% new — smooths out single-block spikes.
        const float  prevLoad = processingLoad.load(std::memory_order_relaxed);
        processingLoad.store(prevLoad * 0.9f + rawLoad * 0.1f, std::memory_order_relaxed);
    }

    // Dark Cockpit B041: compute note activity from output RMS
    {
        float rms = 0.0f;
        if (numSamples > 0)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                    rms += data[i] * data[i];
            }
            rms = std::sqrt(rms / (numSamples * std::max(1, buffer.getNumChannels())));
        }
        // Smooth with ~35ms attack, ~213ms release at 512 samples/48kHz (one-pole filter)
        float current = noteActivity_.load(std::memory_order_relaxed);
        float target = std::clamp(rms * 4.0f, 0.0f, 1.0f); // scale RMS to 0-1 range
        float coeff = (target > current) ? 0.3f : 0.05f; // fast attack, slow release
        noteActivity_.store(current + coeff * (target - current), std::memory_order_relaxed);
    }

    // Emit any pending CC output events from the XOuija UI (lock-free SPSC drain).
    drainCCOutput(midi, numSamples);
}

void XOlokunProcessor::processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs)
{
    // Identify which slots hold Constellation family engines
    static const juce::StringArray kFamilyIds = { "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole" };

    // Collect family engine pointers and their slot indices.
    // familySlots_ is a member pre-allocated in prepareToPlay — no heap alloc here.
    familySlots_.clearQuick();

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;
        if (kFamilyIds.contains(enginePtrs[i]->getEngineId()))
            familySlots_.add({ i, enginePtrs[i] });
    }

    if (familySlots_.size() < 2)
        return; // nothing to bleed

    // Read macro values from cached parameter pointers — safe on audio thread
    const float communeAmt = cachedParams.ohmCommune ? cachedParams.ohmCommune->load() : 0.f;
    const float bondAmt    = cachedParams.obblBond   ? cachedParams.obblBond->load()   : 0.f;
    const float dramaAmt   = cachedParams.oleDrama   ? cachedParams.oleDrama->load()   : 0.f;

    // For each family engine, send bleed coupling to all sibling family engines
    for (const auto& src : familySlots_)
    {
        const float srcSample = src.eng->getSampleForCoupling(0, 0);
        const juce::String srcId = src.eng->getEngineId();

        for (const auto& dst : familySlots_)
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
    {
        const double sr = currentSampleRate.load(std::memory_order_relaxed);
        newEngine->prepare(sr, currentBlockSize.load(std::memory_order_relaxed));
        newEngine->prepareSilenceGate(sr, currentBlockSize.load(std::memory_order_relaxed),
                                      silenceGateHoldMs(newEngine->getEngineId()));
    }

    // Wake the silence gate so the new engine renders its first block immediately.
    // Without this, a freshly-constructed engine's gate is in the bypassed state —
    // any note held across a mid-note swap would produce a one-block DSP gap
    // because processBlock would skip the new engine before its first note-on.
    newEngine->wakeSilenceGate();

    // Post the old engine to the lock-free crossfade mailbox so the audio
    // thread can fade it out without any mutex on the real-time path.
    auto oldEngine = std::atomic_load(&engines[slot]);
    if (oldEngine)
    {
        auto& pending = pendingCrossfades[slot];
        // Wait until the audio thread has consumed any previous pending command
        // for this slot before overwriting the non-atomic fields.  A rapid
        // double-swap (two loadEngine() calls before the audio thread runs)
        // would otherwise produce a data race on outgoing/fadeGain/fadeSamplesRemaining.
        // One audio block at 48 kHz / 512 samples ≈ 10 ms worst-case; safe on the
        // message thread.
        // Bounded wait (max ~50 ms) — prevents indefinite stall on the message
        // thread at large buffer sizes (issue #148).
        for (int i = 0; i < 500 && pending.ready.load(std::memory_order_acquire); ++i)
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        pending.outgoing             = oldEngine;
        pending.fadeGain             = 1.0f;
        pending.fadeSamplesRemaining =
            static_cast<int>(currentSampleRate.load(std::memory_order_relaxed) * CrossfadeMs * 0.001);
        // Release-store: makes the fields above visible to the audio thread
        // before it observes ready==true.
        pending.ready.store(true, std::memory_order_release);
    }

    // Atomic swap — audio thread sees the new engine on next block
    auto shared = std::shared_ptr<SynthEngine>(std::move(newEngine));
    std::atomic_store(&engines[slot], shared);

    // Update coupling matrix with the new engine pointer so it can resolve
    // IAudioBufferSink capabilities and activate/suspend AudioToBuffer routes.
    // Must pass the raw pointer so activeEngines[] is updated before sink resolution.
    // (W2 Audit CRITICAL-1: was reading stale activeEngines without this)
    couplingMatrix.notifyCouplingMatrixOfSwap(slot, engineId, shared.get());

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
        auto& pending = pendingCrossfades[slot];
        // Wait until the audio thread has consumed any previous pending command
        // for this slot before overwriting the non-atomic fields.  Mirrors the
        // spin-check in loadEngine() — same data-race protection.
        // Bounded wait (max ~50 ms) — prevents indefinite stall on the message
        // thread at large buffer sizes (issue #148).
        for (int i = 0; i < 500 && pending.ready.load(std::memory_order_acquire); ++i)
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        pending.outgoing             = oldEngine;
        pending.fadeGain             = 1.0f;
        pending.fadeSamplesRemaining =
            static_cast<int>(currentSampleRate.load(std::memory_order_relaxed) * CrossfadeMs * 0.001);
        // Release-store: makes the fields above visible to the audio thread
        // before it observes ready==true.
        pending.ready.store(true, std::memory_order_release);
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
    // Append supplementary state to the APVTS ValueTree *before* converting
    // to XML so that all extra children are included in the binary blob.
    auto state = apvts.copyState();

    // FIX 5 — Persist MIDI learn mappings as a ValueTree child.
    // Using a structured child (rather than a JSON string attribute) avoids
    // XML attribute string-length limits, survives round-trips through
    // juce::ValueTree::fromXml(), and is the idiomatic JUCE pattern for
    // supplementary plugin state.  The "MIDILearn" child is absent in saves
    // from before this feature; setStateInformation handles that gracefully.
    // Remove any stale MIDILearn child first (defensive — should not exist
    // on a freshly copied state, but guards against double-save edge cases).
    if (auto existing = state.getChildWithName("MIDILearn"); existing.isValid())
        state.removeChild(existing, nullptr);
    state.appendChild(midiLearnManager.toValueTree(), nullptr);

    // XOuija UI state — bank, toggle states, gesture button MIDI learn mappings.
    // onGetXOuijaState is registered by PlaySurface when it connects to this processor.
    // If the PlaySurface window is not open yet, the callback is null and the child
    // is simply omitted (safe: setStateInformation falls back to defaults gracefully).
    if (onGetXOuijaState)
    {
        auto uiState = onGetXOuijaState();
        if (uiState.isValid())
        {
            if (auto existing = state.getChildWithName("XOuijaPanel"); existing.isValid())
                state.removeChild(existing, nullptr);
            state.appendChild(uiState, nullptr);
        }
    }

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

        // FIX 6 — Save slot mute states (closes #311).
        // slotMuted is std::atomic<bool> and was previously not persisted,
        // causing muted slots to revert on DAW session reload.
        for (int i = 0; i < MaxSlots; ++i)
            xml->setAttribute("slotMuted" + juce::String(i), slotMuted[i].load() ? 1 : 0);

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

        copyXmlToBinary(*xml, destData);
    }
}

void XOlokunProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        hasRestoredState = true;  // Mark that saved state exists — skip default-engine load in prepareToPlay.
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

            // FIX 6 — Restore slot mute states (closes #311).
            for (int i = 0; i < MaxSlots; ++i)
                slotMuted[i].store(xml->getIntAttribute("slotMuted" + juce::String(i), 0) != 0);

            // FIX 2 — Restore baseline coupling routes.
            couplingMatrix.clearRoutes();
            if (auto* routesElem = xml->getChildByName("BaselineCouplingRoutes"))
            {
                for (auto* re : routesElem->getChildIterator())
                {
                    MegaCouplingMatrix::CouplingRoute r;
                    r.sourceSlot  = re->getIntAttribute("src", 0);
                    r.destSlot    = re->getIntAttribute("dst", 1);
                    int typeInt = re->getIntAttribute("type", 0);
                    if (typeInt < 0 || typeInt > static_cast<int>(CouplingType::TriangularCoupling))
                        typeInt = 0;
                    r.type = static_cast<CouplingType>(typeInt);
                    r.amount      = static_cast<float>(re->getDoubleAttribute("amount", 0.5));
                    r.isNormalled = re->getBoolAttribute("normalled", false);
                    r.active      = re->getBoolAttribute("active", true);

                    // Bounds-check before adding to prevent corrupted saves
                    // from crashing or creating self-routing loops.
                    // addRoute() also enforces graph-level cycle detection for
                    // audio-rate types, so corrupted saves cannot install a
                    // feedback loop even if the self-route check is bypassed.
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

        // FIX 5 — Restore MIDI learn mappings from the ValueTree child that
        // was appended to the APVTS state in getStateInformation().
        // apvts.replaceState() above has already reconstructed the full tree
        // from XML, so the "MIDILearn" child is now accessible via apvts.state.
        // fromValueTree() returns false when the child is absent (old sessions
        // saved before this feature) or malformed — fall back to defaults so
        // hardware users always have at least the spec-defined CC assignments.
        {
            auto midiLearnTree = apvts.state.getChildWithName("MIDILearn");
            if (!midiLearnManager.fromValueTree(midiLearnTree))
                midiLearnManager.loadDefaultMappings();
        }

        // XOuija UI state — restore active bank, button toggles, gesture MIDI learn.
        // onSetXOuijaState is registered by PlaySurface; if not yet registered (e.g.
        // the PlaySurface window hasn't been opened for the first time), the state
        // tree is stored in apvts.state and PlaySurface will read it on first
        // construction via getChildWithName("XOuijaPanel") in its setProcessor() call.
        {
            auto uiStateTree = apvts.state.getChildWithName("XOuijaPanel");
            if (uiStateTree.isValid() && onSetXOuijaState)
                onSetXOuijaState (uiStateTree);
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

    // Restore coupling routes from preset data.
    // This is the primary differentiator for XOlokun — coupling routes MUST
    // be restored when a preset is applied, or the patch will sound wrong.
    //
    // Strategy:
    //   1. Clear all existing routes.
    //   2. For each CouplingPair, find the slot indices for engineA and engineB
    //      by comparing getEngineId() against the resolved canonical name.
    //   3. Convert the type string to a CouplingType enum value.
    //   4. Add the route if both slots are valid and distinct.
    //
    // Edge cases handled:
    //   - Engine not loaded in any slot → route is skipped (not a crash)
    //   - Self-coupling (sourceSlot == destSlot) → route is skipped
    //   - Unknown type string → route is skipped (already validated by parseJSON,
    //     but we guard here for safety)
    if (!preset.couplingPairs.empty())
    {
        // Helper: convert a validated coupling type string to CouplingType enum.
        // Handles both CamelCase ("AmpToFilter") and arrow-notation
        // ("Amp->Filter") forms — both are accepted by parseJSON/validCouplingTypes.
        auto stringToCouplingType = [](const juce::String& s, CouplingType& out) -> bool
        {
            // CamelCase canonical forms (primary)
            if (s == "AmpToFilter")        { out = CouplingType::AmpToFilter;       return true; }
            if (s == "AmpToPitch")         { out = CouplingType::AmpToPitch;        return true; }
            if (s == "LFOToPitch")         { out = CouplingType::LFOToPitch;        return true; }
            if (s == "EnvToMorph")         { out = CouplingType::EnvToMorph;        return true; }
            if (s == "AudioToFM")          { out = CouplingType::AudioToFM;         return true; }
            if (s == "AudioToRing")        { out = CouplingType::AudioToRing;       return true; }
            if (s == "FilterToFilter")     { out = CouplingType::FilterToFilter;    return true; }
            if (s == "AmpToChoke")         { out = CouplingType::AmpToChoke;        return true; }
            if (s == "RhythmToBlend")      { out = CouplingType::RhythmToBlend;     return true; }
            if (s == "EnvToDecay")         { out = CouplingType::EnvToDecay;        return true; }
            if (s == "PitchToPitch")       { out = CouplingType::PitchToPitch;      return true; }
            if (s == "AudioToWavetable")   { out = CouplingType::AudioToWavetable;  return true; }
            if (s == "AudioToBuffer")      { out = CouplingType::AudioToBuffer;     return true; }
            if (s == "KnotTopology")       { out = CouplingType::KnotTopology;      return true; }
            if (s == "TriangularCoupling") { out = CouplingType::TriangularCoupling; return true; }
            // Legacy arrow-notation aliases
            if (s == "Amp->Filter")        { out = CouplingType::AmpToFilter;       return true; }
            if (s == "Amp->Pitch")         { out = CouplingType::AmpToPitch;        return true; }
            if (s == "LFO->Pitch")         { out = CouplingType::LFOToPitch;        return true; }
            if (s == "Env->Morph")         { out = CouplingType::EnvToMorph;        return true; }
            if (s == "Audio->FM")          { out = CouplingType::AudioToFM;         return true; }
            if (s == "Audio->Ring")        { out = CouplingType::AudioToRing;       return true; }
            if (s == "Filter->Filter")     { out = CouplingType::FilterToFilter;    return true; }
            if (s == "Amp->Choke")         { out = CouplingType::AmpToChoke;        return true; }
            if (s == "Rhythm->Blend")      { out = CouplingType::RhythmToBlend;     return true; }
            if (s == "Env->Decay")         { out = CouplingType::EnvToDecay;        return true; }
            if (s == "Pitch->Pitch")       { out = CouplingType::PitchToPitch;      return true; }
            if (s == "Audio->Wavetable")   { out = CouplingType::AudioToWavetable;  return true; }
            if (s == "Audio->Buffer")      { out = CouplingType::AudioToBuffer;     return true; }
            if (s == "Knot->Topology")     { out = CouplingType::KnotTopology;      return true; }
            if (s == "Triangular->Coupling") { out = CouplingType::TriangularCoupling; return true; }
            return false; // unknown type string — caller should skip this route
        };

        // Build routes into a pending list first — only clear the matrix
        // if we successfully resolve at least one route. This prevents
        // unconditional clearing when a preset references engines that
        // aren't currently loaded (audit finding: P0-1 CRITICAL-2).
        std::vector<MegaCouplingMatrix::CouplingRoute> pendingRoutes;

        for (const auto& cp : preset.couplingPairs)
        {
            // Resolve legacy aliases so old presets still work.
            const juce::String canonA = resolveEngineAlias(cp.engineA);
            const juce::String canonB = resolveEngineAlias(cp.engineB);

            // Find slot indices for both engines by scanning active slots.
            int slotA = -1;
            int slotB = -1;
            for (int i = 0; i < MaxSlots; ++i)
            {
                auto eng = std::atomic_load(&engines[i]);
                if (eng == nullptr)
                    continue;
                const juce::String engId = eng->getEngineId();
                if (slotA < 0 && engId == canonA)
                    slotA = i;
                if (slotB < 0 && engId == canonB)
                    slotB = i;
            }

            if (slotA < 0 || slotB < 0)
            {
                DBG("applyPreset: coupling route skipped — engineA='" + canonA
                    + "' (slot=" + juce::String(slotA) + ") engineB='" + canonB
                    + "' (slot=" + juce::String(slotB) + ") not found in active slots");
                continue;
            }

            if (slotA == slotB)
                continue;

            CouplingType couplingType {};
            if (!stringToCouplingType(cp.type, couplingType))
            {
                DBG("applyPreset: unknown coupling type '" + cp.type + "' — route skipped");
                continue;
            }

            // Preserve bipolar amounts for types that use them (e.g. AmpToPitch,
            // PitchToPitch). Only force unipolar for inherently one-directional types.
            // Audit finding: P0-1 CRITICAL-1 — std::abs() was destroying negative amounts.
            const bool unipolarOnly = (couplingType == CouplingType::AmpToChoke
                                    || couplingType == CouplingType::AudioToBuffer
                                    || couplingType == CouplingType::AudioToWavetable);
            const float clampedAmount = unipolarOnly
                ? juce::jlimit(0.0f, 1.0f, std::abs(cp.amount))
                : juce::jlimit(-1.0f, 1.0f, cp.amount);

            MegaCouplingMatrix::CouplingRoute route;
            route.sourceSlot  = slotA;
            route.destSlot    = slotB;
            route.type        = couplingType;
            route.amount      = clampedAmount;
            route.isNormalled = false;
            route.active      = true;

            // Cycle detection: skip any audio-rate route that would form a
            // directed feedback loop (A→B→A) in the pending route set.
            //
            // We build a temporary scratch matrix, add the routes accepted so
            // far, and ask wouldCreateCycle() whether this new route closes a
            // cycle. This is correct because pendingRoutes will replace the
            // current matrix entirely on commit — we must check against the
            // routes we are about to install, not the stale live matrix.
            {
                MegaCouplingMatrix scratchMatrix;
                for (const auto& accepted : pendingRoutes)
                    scratchMatrix.addRoute(accepted);

                if (scratchMatrix.wouldCreateCycle(route.sourceSlot,
                                                   route.destSlot,
                                                   route.type))
                {
                    DBG("applyPreset: audio-rate cycle detected — skipping route "
                        + canonA + "(slot " + juce::String(slotA) + ") → "
                        + canonB + "(slot " + juce::String(slotB) + ") ["
                        + cp.type + "]");
                    continue;
                }
            }

            pendingRoutes.push_back(route);
        }

        // Only clear + replace routes if we resolved at least one, OR if the
        // preset explicitly has no coupling pairs (intentional uncoupled preset).
        if (!pendingRoutes.empty() || preset.couplingPairs.empty())
        {
            couplingMatrix.clearRoutes();
            for (const auto& r : pendingRoutes)
                couplingMatrix.addRoute(r);
        }
        else if (!preset.couplingPairs.empty())
        {
            DBG("applyPreset: " + juce::String((int)preset.couplingPairs.size())
                + " coupling routes could not be restored — required engines not loaded. "
                + "Existing coupling routes preserved.");
        }
    }
}

} // namespace xolokun

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new xolokun::XOlokunProcessor();
}
