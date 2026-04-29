#include "XOceanusProcessor.h"
// XOceanusEditor.h is intentionally NOT included here. createEditor() is
// implemented in Source/UI/XOceanusEditor.cpp to keep the UI include chain
// (ExportDialog → XPNVelocityCurves, CouplingVisualizer → GalleryColors, etc.)
// out of this translation unit and avoid circular include complications.
#include "Engines/OddfeliX/OddfeliXEngine.h"
#include "Engines/OddOscar/OddOscarEngine.h"
#include "Engines/Overdub/OverdubEngine.h"
#include "Engines/Odyssey/OdysseyEngine.h"
#include "Engines/Oblong/OblongEngine.h"
#include "Engines/Obese/ObeseEngine.h"
#include "Engines/Onset/OnsetEngine.h"
#include "Engines/Overworld/OverworldEngine.h"
#include "Engines/Opal/OpalEngine.h"
#include "Engines/Overbite/OverbiteEngine.h"
#include "Engines/Organon/OrganonEngine.h"
#include "Engines/Ocelot/OcelotEngine.h"
#include "Engines/Ouroboros/OuroborosEngine.h"
#include "Engines/Obsidian/ObsidianEngine.h"
#include "Engines/Observandum/ObservandumEngine.h"
#include "Engines/Orrery/OrreryEngine.h"
#include "Engines/Opsin/OpsinEngine.h"
#include "Engines/Oort/OortEngine.h"
#include "Engines/Ondine/OndineEngine.h"
#include "Engines/Ortolan/OrtolanEngine.h"
#include "Engines/Octant/OctantEngine.h"
#include "Engines/Overtide/OvertideEngine.h"
#include "Engines/Oobleck/OobleckEngine.h"
#include "Engines/Ooze/OozeEngine.h"
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
#include "Engines/OceanDeep/OceanDeepEngine.h"
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
// OXIDIZE — age-based corrosion synthesis engine
#include "Engines/Oxidize/OxidizeAdapter.h"
// OGIVE — scanned glass synthesis (engine #86)
#include "Engines/Ogive/OgiveEngine.h"
// OLVIDO — spectral erosion synthesis (engine #87)
#include "Engines/Olvido/OlvidoEngine.h"
// OSTRACON — corpus-buffer synthesis (engine #88)
#include "Engines/Ostracon/OstraconEngine.h"
// OUTCROP — wave-terrain synthesis (engine #89)
#include "Engines/Outcrop/OutcropEngine.h"
// OLLOTRON — tape-chamber keyboard synthesis (engine #91)
#include "Engines/Ollotron/OllotronEngine.h"
// ONDA — NLS soliton synthesis (engine #92; formerly Oneiric)
#include "Engines/Onda/OndaEngine.h"
#include "DSP/Effects/MathFXChain.h"
#include "DSP/Effects/BoutiqueFXChain.h"
#include "DSP/Effects/AquaticFXSuite.h"
#include "Core/EpicChainSlotController.h"
#include "DSP/ThreadInit.h"
#include <cstring> // std::strncmp — used in Wave 5 A1 global mod route evaluation

// Register engines with their canonical IDs (matching getEngineId() return values).
// These MUST match the string returned by each engine's getEngineId().
// Legacy names ("Snap", "Morph", etc.) are resolved by resolveEngineAlias() in PresetManager.h.
static bool registered_OddfeliX = xoceanus::EngineRegistry::instance().registerEngine(
    "OddfeliX", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::SnapEngine>(); });
static bool registered_OddOscar = xoceanus::EngineRegistry::instance().registerEngine(
    "OddOscar", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::MorphEngine>(); });
static bool registered_Overdub = xoceanus::EngineRegistry::instance().registerEngine(
    "Overdub", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::DubEngine>(); });
static bool registered_Odyssey = xoceanus::EngineRegistry::instance().registerEngine(
    "Odyssey", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::DriftEngine>(); });
static bool registered_Oblong = xoceanus::EngineRegistry::instance().registerEngine(
    "Oblong", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::BobEngine>(); });
static bool registered_Obese = xoceanus::EngineRegistry::instance().registerEngine(
    "Obese", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::FatEngine>(); });
static bool registered_Onset = xoceanus::EngineRegistry::instance().registerEngine(
    "Onset", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OnsetEngine>(); });
static bool registered_Overworld =
    xoceanus::EngineRegistry::instance().registerEngine("Overworld", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OverworldEngine>(); });
static bool registered_Opal = xoceanus::EngineRegistry::instance().registerEngine(
    "Opal", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OpalEngine>(); });
static bool registered_Overbite = xoceanus::EngineRegistry::instance().registerEngine(
    "Overbite", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::BiteEngine>(); });
static bool registered_Organon = xoceanus::EngineRegistry::instance().registerEngine(
    "Organon", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OrganonEngine>(); });
static bool registered_Ocelot = xoceanus::EngineRegistry::instance().registerEngine(
    "Ocelot", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xocelot::OcelotEngine>(); });
static bool registered_Ouroboros =
    xoceanus::EngineRegistry::instance().registerEngine("Ouroboros", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OuroborosEngine>(); });
static bool registered_Obsidian =
    xoceanus::EngineRegistry::instance().registerEngine("Obsidian", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::ObsidianEngine>(); });
static bool registered_Observandum =
    xoceanus::EngineRegistry::instance().registerEngine("Observandum", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::ObservandumEngine>(); });
static bool registered_Orrery =
    xoceanus::EngineRegistry::instance().registerEngine("Orrery", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OrreryEngine>(); });
static bool registered_Opsin =
    xoceanus::EngineRegistry::instance().registerEngine("Opsin", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OpsinEngine>(); });
static bool registered_Oort =
    xoceanus::EngineRegistry::instance().registerEngine("Oort", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OortEngine>(); });
static bool registered_Ondine =
    xoceanus::EngineRegistry::instance().registerEngine("Ondine", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OndineEngine>(); });
static bool registered_Ortolan =
    xoceanus::EngineRegistry::instance().registerEngine("Ortolan", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OrtolanEngine>(); });
static bool registered_Octant =
    xoceanus::EngineRegistry::instance().registerEngine("Octant", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OctantEngine>(); });
static bool registered_Overtide =
    xoceanus::EngineRegistry::instance().registerEngine("Overtide", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OvertideEngine>(); });
static bool registered_Oobleck =
    xoceanus::EngineRegistry::instance().registerEngine("Oobleck", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OobleckEngine>(); });
static bool registered_Ooze =
    xoceanus::EngineRegistry::instance().registerEngine("Ooze", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OozeEngine>(); });
static bool registered_Origami = xoceanus::EngineRegistry::instance().registerEngine(
    "Origami", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OrigamiEngine>(); });
static bool registered_Oracle = xoceanus::EngineRegistry::instance().registerEngine(
    "Oracle", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OracleEngine>(); });
static bool registered_Obscura = xoceanus::EngineRegistry::instance().registerEngine(
    "Obscura", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::ObscuraEngine>(); });
static bool registered_Oceanic = xoceanus::EngineRegistry::instance().registerEngine(
    "Oceanic", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OceanicEngine>(); });
static bool registered_Optic = xoceanus::EngineRegistry::instance().registerEngine(
    "Optic", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OpticEngine>(); });
static bool registered_Oblique = xoceanus::EngineRegistry::instance().registerEngine(
    "Oblique", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::ObliqueEngine>(); });
static bool registered_Orbital = xoceanus::EngineRegistry::instance().registerEngine(
    "Orbital", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OrbitalEngine>(); });
static bool registered_Osprey = xoceanus::EngineRegistry::instance().registerEngine(
    "Osprey", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OspreyEngine>(); });
static bool registered_Osteria = xoceanus::EngineRegistry::instance().registerEngine(
    "Osteria", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OsteriaEngine>(); });
static bool registered_Owlfish = xoceanus::EngineRegistry::instance().registerEngine(
    "Owlfish", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xowlfish::OwlfishEngine>(); });
static bool registered_Ohm = xoceanus::EngineRegistry::instance().registerEngine(
    "Ohm", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OhmEngine>(); });
static bool registered_Orphica = xoceanus::EngineRegistry::instance().registerEngine(
    "Orphica", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OrphicaEngine>(); });
static bool registered_Obbligato =
    xoceanus::EngineRegistry::instance().registerEngine("Obbligato", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::ObbligatoEngine>(); });
static bool registered_Ottoni = xoceanus::EngineRegistry::instance().registerEngine(
    "Ottoni", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OttoniEngine>(); });
static bool registered_Ole = xoceanus::EngineRegistry::instance().registerEngine(
    "Ole", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OleEngine>(); });
static bool registered_Overlap = xoceanus::EngineRegistry::instance().registerEngine(
    "Overlap", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::XOverlapEngine>(); });
static bool registered_Outwit = xoceanus::EngineRegistry::instance().registerEngine(
    "Outwit", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::XOutwitEngine>(); });
static bool registered_Ombre = xoceanus::EngineRegistry::instance().registerEngine(
    "Ombre", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OmbreEngine>(); });
static bool registered_Orca = xoceanus::EngineRegistry::instance().registerEngine(
    "Orca", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OrcaEngine>(); });
static bool registered_Octopus = xoceanus::EngineRegistry::instance().registerEngine(
    "Octopus", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OctopusEngine>(); });
// Concept Engines — OPENSKY
static bool registered_OpenSky = xoceanus::EngineRegistry::instance().registerEngine(
    "OpenSky", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OpenSkyEngine>(); });
// Concept Engines — OSTINATO
static bool registered_Ostinato =
    xoceanus::EngineRegistry::instance().registerEngine("Ostinato", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OstinatoEngine>(); });
// Concept Engines — OCEANDEEP
static bool registered_OceanDeep =
    xoceanus::EngineRegistry::instance().registerEngine("OceanDeep", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OceandeepEngine>(); });
// Concept Engines — OUIE
static bool registered_Ouie = xoceanus::EngineRegistry::instance().registerEngine(
    "Ouie", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OuieEngine>(); });
// Flagship — OBRIX (modular brick synthesis)
static bool registered_Obrix = xoceanus::EngineRegistry::instance().registerEngine(
    "Obrix", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::ObrixEngine>(); });
// Theorem Engine — ORBWEAVE (topological knot coupling)
static bool registered_Orbweave =
    xoceanus::EngineRegistry::instance().registerEngine("Orbweave", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OrbweaveEngine>(); });
// Theorem Engine — OVERTONE (continued fraction spectral synthesis)
static bool registered_Overtone =
    xoceanus::EngineRegistry::instance().registerEngine("Overtone", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OvertoneEngine>(); });
// Theorem Engine — ORGANISM (cellular automata generative synthesis)
static bool registered_Organism =
    xoceanus::EngineRegistry::instance().registerEngine("Organism", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OrganismEngine>(); });
// Singularity Collection — OXBOW (entangled reverb synth engine)
static bool registered_Oxbow = xoceanus::EngineRegistry::instance().registerEngine(
    "Oxbow", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OxbowEngine>(); });
// OWARE — tuned percussion synthesizer (wood/metal material continuum)
static bool registered_Oware = xoceanus::EngineRegistry::instance().registerEngine(
    "Oware", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OwareEngine>(); });
// OPERA — additive-vocal Kuramoto synchronicity engine (Humpback Whale / SOFAR)
static bool registered_Opera = xoceanus::EngineRegistry::instance().registerEngine(
    "Opera", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OperaAdapter>(); });
// OFFERING — psychology-driven boom bap drum synthesis (Mantis Shrimp / Rubble Zone)
static bool registered_Offering =
    xoceanus::EngineRegistry::instance().registerEngine("Offering", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OfferingEngine>(); });
// Chef Quad Collection — OTO (tape & circuit-bent heritage)
static bool registered_Oto = xoceanus::EngineRegistry::instance().registerEngine(
    "Oto", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OtoEngine>(); });
// Chef Quad Collection — OCTAVE (harmonic interval synthesis)
static bool registered_Octave = xoceanus::EngineRegistry::instance().registerEngine(
    "Octave", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OctaveEngine>(); });
// Chef Quad Collection — OLEG (folk/modal string synthesis)
static bool registered_Oleg = xoceanus::EngineRegistry::instance().registerEngine(
    "Oleg", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OlegEngine>(); });
// Chef Quad Collection — OTIS (soul/funk synthesis)
static bool registered_Otis = xoceanus::EngineRegistry::instance().registerEngine(
    "Otis", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OtisEngine>(); });
// KITCHEN Quad Collection — OVEN
static bool registered_Oven = xoceanus::EngineRegistry::instance().registerEngine(
    "Oven", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OvenEngine>(); });
// KITCHEN Quad Collection — OCHRE
static bool registered_Ochre = xoceanus::EngineRegistry::instance().registerEngine(
    "Ochre", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OchreEngine>(); });
// KITCHEN Quad Collection — OBELISK
static bool registered_Obelisk = xoceanus::EngineRegistry::instance().registerEngine(
    "Obelisk", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::ObeliskEngine>(); });
// KITCHEN Quad Collection — OPALINE
static bool registered_Opaline = xoceanus::EngineRegistry::instance().registerEngine(
    "Opaline", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OpalineEngine>(); });
// CELLAR Quad Collection — OGRE
static bool registered_Ogre = xoceanus::EngineRegistry::instance().registerEngine(
    "Ogre", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OgreEngine>(); });
// CELLAR Quad Collection — OLATE
static bool registered_Olate = xoceanus::EngineRegistry::instance().registerEngine(
    "Olate", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OlateEngine>(); });
// CELLAR Quad Collection — OAKEN
static bool registered_Oaken = xoceanus::EngineRegistry::instance().registerEngine(
    "Oaken", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OakenEngine>(); });
// CELLAR Quad Collection — OMEGA
static bool registered_Omega = xoceanus::EngineRegistry::instance().registerEngine(
    "Omega", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OmegaEngine>(); });
// GARDEN Quad Collection — ORCHARD
static bool registered_Orchard = xoceanus::EngineRegistry::instance().registerEngine(
    "Orchard", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OrchardEngine>(); });
// GARDEN Quad Collection — OVERGROW
static bool registered_Overgrow =
    xoceanus::EngineRegistry::instance().registerEngine("Overgrow", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OvergrowEngine>(); });
// GARDEN Quad Collection — OSIER
static bool registered_Osier = xoceanus::EngineRegistry::instance().registerEngine(
    "Osier", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OsierEngine>(); });
// GARDEN Quad Collection — OXALIS
static bool registered_Oxalis = xoceanus::EngineRegistry::instance().registerEngine(
    "Oxalis", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OxalisEngine>(); });
// BROTH Quad Collection — OVERWASH
static bool registered_Overwash =
    xoceanus::EngineRegistry::instance().registerEngine("Overwash", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OverwashEngine>(); });
// BROTH Quad Collection — OVERWORN
static bool registered_Overworn =
    xoceanus::EngineRegistry::instance().registerEngine("Overworn", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OverwornEngine>(); });
// BROTH Quad Collection — OVERFLOW
static bool registered_Overflow =
    xoceanus::EngineRegistry::instance().registerEngine("Overflow", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OverflowEngine>(); });
// BROTH Quad Collection — OVERCAST
static bool registered_Overcast =
    xoceanus::EngineRegistry::instance().registerEngine("Overcast", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OvercastEngine>(); });
// FUSION Quad Collection — OKEANOS (formerly Oasis, renamed to free ID for ecosystem engine)
static bool registered_Okeanos = xoceanus::EngineRegistry::instance().registerEngine(
    "Okeanos", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OkeanosEngine>(); });
// FUSION Quad Collection — OASIS (bioluminescent ecosystem engine)
static bool registered_Oasis = xoceanus::EngineRegistry::instance().registerEngine(
    "Oasis", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OasisEngine>(); });
// FUSION Quad Collection — ODDFELLOW
static bool registered_Oddfellow =
    xoceanus::EngineRegistry::instance().registerEngine("Oddfellow", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OddfellowEngine>(); });
// FUSION Quad Collection — ONKOLO
static bool registered_Onkolo = xoceanus::EngineRegistry::instance().registerEngine(
    "Onkolo", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OnkoloEngine>(); });
// FUSION Quad Collection — OPCODE
static bool registered_Opcode = xoceanus::EngineRegistry::instance().registerEngine(
    "Opcode", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OpcodeEngine>(); });
// Membrane Collection — OSMOSIS (engine #47, external audio membrane)
static bool registered_Osmosis = xoceanus::EngineRegistry::instance().registerEngine(
    "Osmosis", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OsmosisEngine>(); });
// OXYTOCIN — circuit-modeling love-triangle synth, Engine #48 (Synapse Violet)
static bool registered_Oxytocin =
    xoceanus::EngineRegistry::instance().registerEngine("Oxytocin", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OxytocinAdapter>(); });
// OUTLOOK — panoramic visionary synth (Albatross / Surface Soarer)
static bool registered_Outlook = xoceanus::EngineRegistry::instance().registerEngine(
    "Outlook", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OutlookEngine>(); });
// Dual Engine Integration — OUTFLOW (Predictive Spatial Vacuum, engine #49)
static bool registered_Outflow = xoceanus::EngineRegistry::instance().registerEngine(
    "Outflow", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OutflowEngine>(); });
// Cellular Automata Oscillator — OBIONT (engine #74)
static bool registered_Obiont = xoceanus::EngineRegistry::instance().registerEngine(
    "Obiont", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::ObiontEngine>(); });
// OXIDIZE — age-based corrosion synthesis engine
static bool registered_Oxidize = xoceanus::EngineRegistry::instance().registerEngine(
    "Oxidize", []() -> std::unique_ptr<xoceanus::SynthEngine> { return std::make_unique<xoceanus::OxidizeAdapter>(); });
// OGIVE — scanned glass synthesis (engine #86)
static bool registered_Ogive =
    xoceanus::EngineRegistry::instance().registerEngine("Ogive", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OgiveEngine>(); });
// OLVIDO — spectral erosion synthesis (engine #87)
static bool registered_Olvido =
    xoceanus::EngineRegistry::instance().registerEngine("Olvido", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OlvidoEngine>(); });

// OSTRACON — corpus-buffer synthesis (engine #88)
static bool registered_Ostracon =
    xoceanus::EngineRegistry::instance().registerEngine("Ostracon", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OstraconEngine>(); });

// OUTCROP — wave-terrain synthesis (engine #89)
static bool registered_Outcrop =
    xoceanus::EngineRegistry::instance().registerEngine("Outcrop", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OutcropEngine>(); });

// ONDA — NLS soliton synthesis (engine #90; formerly Oneiric)
static bool registered_Onda =
    xoceanus::EngineRegistry::instance().registerEngine("Onda", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OndaEngine>(); });

// OLLOTRON — tape-chamber keyboard synthesis (engine #91)
static bool registered_Ollotron =
    xoceanus::EngineRegistry::instance().registerEngine("Ollotron", []() -> std::unique_ptr<xoceanus::SynthEngine>
                                                        { return std::make_unique<xoceanus::OllotronEngine>(); });

namespace xoceanus
{

XOceanusProcessor::XOceanusProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "XOceanusParams", createParameterLayout()), macroSystem_(apvts),
      couplingPresetManager(apvts,
                            [this](int slot) -> juce::String
                            {
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

XOceanusProcessor::~XOceanusProcessor() = default;

bool XOceanusProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    const auto& inputSet = layouts.getMainInputChannelSet();
    if (inputSet != juce::AudioChannelSet::disabled() && inputSet != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void XOceanusProcessor::cacheParameterPointers()
{
    cachedParams.masterVolume = apvts.getRawParameterValue("masterVolume");
    cachedParams.cmEnabled = apvts.getRawParameterValue("cm_enabled");
    cachedParams.cmPalette = apvts.getRawParameterValue("cm_palette");
    cachedParams.cmVoicing = apvts.getRawParameterValue("cm_voicing");
    cachedParams.cmSpread = apvts.getRawParameterValue("cm_spread");
    cachedParams.cmSeqRunning = apvts.getRawParameterValue("cm_seq_running");
    cachedParams.cmSeqBpm = apvts.getRawParameterValue("cm_seq_bpm");
    cachedParams.cmSeqSwing = apvts.getRawParameterValue("cm_seq_swing");
    cachedParams.cmSeqGate = apvts.getRawParameterValue("cm_seq_gate");
    cachedParams.cmSeqPattern = apvts.getRawParameterValue("cm_seq_pattern");
    cachedParams.cmVelCurve = apvts.getRawParameterValue("cm_vel_curve");
    cachedParams.cmHumanize = apvts.getRawParameterValue("cm_humanize");
    cachedParams.cmSidechainDuck = apvts.getRawParameterValue("cm_sidechain_duck");
    cachedParams.cmEnoMode = apvts.getRawParameterValue("cm_eno_mode");
    // Per-slot chord/seq routing (Wave 5 B3)
    for (int slot = 0; slot < 4; ++slot)
        cachedParams.cmSlotRoute[slot] = apvts.getRawParameterValue("cm_slot_route_" + juce::String(slot));

    // B2: input mode + global key/scale + 48 pad chord params
    cachedParams.cmInputMode   = apvts.getRawParameterValue("chord_input_mode");
    cachedParams.cmGlobalRoot  = apvts.getRawParameterValue("cm_global_root");
    cachedParams.cmGlobalScale = apvts.getRawParameterValue("cm_global_scale");
    for (int i = 0; i < 16; ++i)
    {
        const juce::String prefix = "chord_pad_" + juce::String(i) + "_";
        cachedParams.padChords[i].root    = apvts.getRawParameterValue(prefix + "root");
        cachedParams.padChords[i].voicing = apvts.getRawParameterValue(prefix + "voicing");
        cachedParams.padChords[i].inv     = apvts.getRawParameterValue(prefix + "inv");
        jassert(cachedParams.padChords[i].root    != nullptr);
        jassert(cachedParams.padChords[i].voicing != nullptr);
        jassert(cachedParams.padChords[i].inv     != nullptr);
    }

    cachedParams.ohmCommune = apvts.getRawParameterValue("ohm_macroCommune");
    cachedParams.obblBond = apvts.getRawParameterValue("obbl_macroBond");
    cachedParams.oleDrama = apvts.getRawParameterValue("ole_macroDrama");

    // Coupling performance overlay — cache all 20 params for audio-thread access
    for (int r = 0; r < CouplingCrossfader::MaxRouteSlots; ++r)
    {
        const auto prefix = "cp_r" + juce::String(r + 1) + "_";
        cachedParams.cpRoutes[static_cast<size_t>(r)].active = apvts.getRawParameterValue(prefix + "active");
        cachedParams.cpRoutes[static_cast<size_t>(r)].type = apvts.getRawParameterValue(prefix + "type");
        cachedParams.cpRoutes[static_cast<size_t>(r)].amount = apvts.getRawParameterValue(prefix + "amount");
        cachedParams.cpRoutes[static_cast<size_t>(r)].source = apvts.getRawParameterValue(prefix + "source");
        cachedParams.cpRoutes[static_cast<size_t>(r)].target = apvts.getRawParameterValue(prefix + "target");

        // B3: Assert that all parameter pointers resolved — catches typos in param IDs.
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].active != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].type != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].amount != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].source != nullptr);
        jassert(cachedParams.cpRoutes[static_cast<size_t>(r)].target != nullptr);
    }

    // MPE params
    cachedParams.mpeEnabled = apvts.getRawParameterValue("mpe_enabled");
    cachedParams.mpeZone = apvts.getRawParameterValue("mpe_zone");
    cachedParams.mpePitchBendRange = apvts.getRawParameterValue("mpe_pitchBendRange");
    cachedParams.mpePressureTarget = apvts.getRawParameterValue("mpe_pressureTarget");
    cachedParams.mpeSlideTarget = apvts.getRawParameterValue("mpe_slideTarget");

    // Wave 5 D1: XOuija walk engine mood/tendency — cache once, read per-block
    cachedParams.ouijaCalmWild           = apvts.getRawParameterValue("ouija_calm_wild");
    cachedParams.ouijaConsonantDissonant = apvts.getRawParameterValue("ouija_consonant_dissonant");
    cachedParams.ouijaTendencyCol        = apvts.getRawParameterValue("ouija_tendency_col");
    cachedParams.ouijaTendencyRow        = apvts.getRawParameterValue("ouija_tendency_row");

    // B1: Pre-resolve the Orrery cutoff parameter pointer used for isOrryCutoff detection
    // in flushModRoutesSnapshot().  If the Orrery engine is not registered, this stays
    // nullptr and isOrryCutoff is never set (safe — globalCutoffModOffset_ stays 0.0f).
    orryCutoffParam_ = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter("orry_fltCutoff"));
}

juce::AudioProcessorValueTreeState::ParameterLayout XOceanusProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterVolume", 1), "Master Volume",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    // 4 Macro knobs (CHARACTER, MOVEMENT, COUPLING, SPACE)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro1", 1), "CHARACTER",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro2", 1), "MOVEMENT",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro3", 1), "COUPLING",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro4", 1), "SPACE",
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
    // Concept Engines
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
    // OUTCROP — wave-terrain synthesis (engine #89)
    OutcropEngine::addParameters(params);
    // OLLOTRON — tape-chamber keyboard synthesis (engine #91)
    xoceanus::OllotronEngine::addParameters(params);
    // ONDA — NLS soliton synthesis (engine #92; formerly Oneiric)
    OndaEngine::addParameters(params);

    // ── Three FX suites wired as optional stages in MasterFXChain (issue #153) ──
    // MathFXChain and BoutiqueFXChain use the shared params vector.
    // All mix parameters default to 0 — bypassed at zero CPU until user enables them.
    xoceanus::MathFXChain::addParameters(params);
    xoceanus::BoutiqueFXChain::addParameters(params);

    // ── Coupling Performance Overlay ──────────────────────────────────────────
    // 4 route slots × 5 params = 20 new APVTS parameters.
    // These are ephemeral live-performance controls that overlay preset coupling.
    // See Docs/specs/coupling_performance_spec.md §2.2.
    {
        // CouplingType enum labels (0–14) — must match CouplingType order in SynthEngine.h
        const juce::StringArray couplingTypeLabels{
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

        const juce::StringArray slotLabels{"Slot 1", "Slot 2", "Slot 3", "Slot 4"};

        // Default target for each route: r1→1, r2→1, r3→2, r4→3
        const int defaultTargets[4] = {1, 1, 2, 3};

        for (int r = 0; r < 4; ++r)
        {
            const auto prefix = "cp_r" + juce::String(r + 1) + "_";

            // Active (bool, default off)
            params.push_back(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID(prefix + "active", 1), "CP Route " + juce::String(r + 1) + " Active", false));

            // Type (int choice 0-13, default 0 = AmpToFilter)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(prefix + "type", 1),
                                                                          "CP Route " + juce::String(r + 1) + " Type",
                                                                          couplingTypeLabels, 0));

            // Amount (float, bipolar -1.0 to 1.0, default 0.0)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID(prefix + "amount", 1), "CP Route " + juce::String(r + 1) + " Amount",
                juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

            // Source (int choice 0-3, default 0 = Slot 1)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID(prefix + "source", 1), "CP Route " + juce::String(r + 1) + " Source", slotLabels, 0));

            // Target (int choice 0-3, default varies per route)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(prefix + "target", 1),
                                                                          "CP Route " + juce::String(r + 1) + " Target",
                                                                          slotLabels, defaultTargets[r]));
        }
    }

    // Chord Machine parameters
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(juce::ParameterID("cm_enabled", 1), "Chord Machine", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_palette", 1), "CM Palette",
        juce::StringArray{"WARM", "BRIGHT", "TENSION", "OPEN", "DARK", "SWEET", "COMPLEX", "RAW"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_voicing", 1), "CM Voicing",
        juce::StringArray{
            // Tertian (0-4) — indices fixed for preset backward compat
            "ROOT-SPREAD", "DROP-2", "QUARTAL", "UPPER STRUCT", "UNISON",
            // Quartal family (5-6)
            "QUARTAL-3", "QUARTAL-4",
            // Quintal family (7-8)
            "QUINTAL-3", "QUINTAL-4",
            // Modal-world family (9-13)
            "HIJAZ", "BHAIRAVI", "YO", "IN", "PHRYG-DOM",
            // Drone family (14-19)
            "DRONE-P5", "DRONE-P4", "DRONE-M3", "DRONE-m3", "DRONE-M2", "DRONE-m2"
        }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cm_spread", 1), "CM Spread",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f));
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(juce::ParameterID("cm_seq_running", 1), "CM Sequencer", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cm_seq_bpm", 1), "CM BPM", juce::NormalisableRange<float>(30.0f, 300.0f, 0.1f), 122.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cm_seq_swing", 1), "CM Swing",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cm_seq_gate", 1), "CM Gate",
                                                                 juce::NormalisableRange<float>(0.01f, 1.0f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_seq_pattern", 1), "CM Pattern",
        juce::StringArray{"FOUR", "OFF-BEAT", "SYNCO", "STAB", "GATE", "PULSE", "BROKEN", "REST"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_vel_curve", 1), "CM Velocity Curve",
        juce::StringArray{"EQUAL", "ROOT HEAVY", "TOP BRIGHT", "V-SHAPE"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cm_humanize", 1), "CM Humanize",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cm_sidechain_duck", 1),
                                                                 "CM Sidechain Duck",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(juce::ParameterID("cm_eno_mode", 1), "CM Eno Mode", false));

    // Per-slot chord/seq routing mode (Wave 5 B3).
    // One Choice parameter per primary engine slot (slots 0–3).
    // Values: 0=CHORD→SEQ, 1=SEQ→CHORD, 2=PARALLEL. Default: CHORD→SEQ.
    for (int slot = 0; slot < 4; ++slot)
    {
        const juce::String paramId  = "cm_slot_route_" + juce::String(slot);
        const juce::String paramName = "CM Slot " + juce::String(slot + 1) + " Routing";
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(paramId, 1), paramName,
            juce::StringArray{"CHORD→SEQ", "SEQ→CHORD", "PARALLEL"}, 0));
    }

    // ── B3: Per-slot chord input mode (Wave 5 B3 mount) ──────────────────────
    // One Choice parameter per primary engine slot (slots 0–3).
    // Values: 0=AUTO-HARMONIZE, 1=PAD-PER-CHORD, 2=SCALE-DEGREE. Default: AUTO-HARMONIZE.
    for (int slot = 0; slot < 4; ++slot)
    {
        const juce::String paramId  = "cm_slot_input_mode_" + juce::String(slot);
        const juce::String paramName = "CM Slot " + juce::String(slot + 1) + " Input Mode";
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(paramId, 1), paramName,
            juce::StringArray{"AUTO-HARMONIZE", "PAD-PER-CHORD", "SCALE-DEGREE"}, 0));
    }

    // ── B2: Chord input mode + global key/scale ───────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("chord_input_mode", 1), "Chord Input Mode",
        juce::StringArray{"AUTO", "PAD", "DEG"}, 0)); // default = AutoHarmonize

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_global_root", 1), "CM Global Root",
        juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0)); // C

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("cm_global_scale", 1), "CM Global Scale",
        juce::StringArray{"Chromatic","Major","Minor","Dorian","Mixolydian",
                          "Pent Min","Pent Maj","Blues","Harm Min"}, 1)); // Major

    // ── B2: Pad chord slots — 16 pads × 3 params = 48 params ─────────────────
    // Build the voicing string array once (reused for all 16 pads)
    {
        juce::StringArray voicingChoices{
            "ROOT-SPREAD","DROP-2","QUARTAL","UPPER-STRUCT","UNISON",
            "QUARTAL-3","QUARTAL-4","QUINTAL-3","QUINTAL-4",
            "HIJAZ","BHAIRAVI","YO","IN","PHRYG-DOM",
            "DRONE-P5","DRONE-P4","DRONE-M3","DRONE-m3","DRONE-M2","DRONE-m2"
        };

        // Default roots follow the C major scale degrees for pads 0-7:
        // C D E F G A B C (pads 0-7), then repeat for pads 8-15
        static constexpr int kDefaultRoots[16] = {
            60, 62, 64, 65, 67, 69, 71, 72,  // C4 D4 E4 F4 G4 A4 B4 C5
            60, 62, 64, 65, 67, 69, 71, 72   // repeat (pads 8-15)
        };

        for (int i = 0; i < 16; ++i)
        {
            const juce::String prefix = "chord_pad_" + juce::String(i) + "_";

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(prefix + "root", 1),
                "Chord Pad " + juce::String(i + 1) + " Root",
                0, 127, kDefaultRoots[i]));

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID(prefix + "voicing", 1),
                "Chord Pad " + juce::String(i + 1) + " Voicing",
                voicingChoices, 0)); // default = RootSpread

            params.push_back(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(prefix + "inv", 1),
                "Chord Pad " + juce::String(i + 1) + " Inversion",
                0, 3, 0)); // default = root position
        }
    }

    // Master FX parameters
    // Stage 1: Saturation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_satDrive", 1),
                                                                 "Master Sat Drive",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_satMode", 1), "Master Sat Mode",
                                                    juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 1.0f));

    // Stage 2: Corroder (digital erosion)
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_corrMix", 1), "Master Corroder Mix",
                                                    juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f),
                                                    0.0f)); // skew 0.5: more resolution in subtle blend range
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_corrBits", 1), "Master Corroder Bits",
                                                    juce::NormalisableRange<float>(1.0f, 24.0f, 0.0f, 0.5f), 24.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrSR", 1), "Master Corroder SR",
        juce::NormalisableRange<float>(100.0f, 44100.0f, 0.0f, 0.3f), 44100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_corrFM", 1), "Master Corroder FM", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_corrTone", 1),
                                                                 "Master Corroder Tone",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // Stage 4: Combulator (tuned comb bank)
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combMix", 1), "Master Comb Mix",
                                                    juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f),
                                                    0.0f)); // skew 0.5: more resolution in subtle blend range
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_combFreq", 1), "Master Comb Freq",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.0f, 0.3f), 220.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combFeedback", 1),
                                                                 "Master Comb Feedback",
                                                                 juce::NormalisableRange<float>(0.0f, 0.98f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combDamping", 1),
                                                                 "Master Comb Damping",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combNoise", 1),
                                                                 "Master Comb Noise",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combSpread", 1),
                                                                 "Master Comb Spread",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combOffset2", 1), "Master Comb Offset 2",
                                                    juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 7.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_combOffset3", 1), "Master Comb Offset 3",
                                                    juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 12.0f));

    // Stage 6: Frequency Shifter
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_fshiftHz", 1), "Master Freq Shift Hz",
                                                    juce::NormalisableRange<float>(-1000.0f, 1000.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_fshiftMix", 1), "Master Freq Shift Mix",
                                                    juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f),
                                                    0.0f)); // skew 0.5: more resolution in subtle blend range
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_fshiftMode", 1), "Master Freq Shift Mode",
                                                    juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_fshiftFeedback", 1),
                                                                 "Master Freq Shift FB",
                                                                 juce::NormalisableRange<float>(0.0f, 0.9f), 0.0f));

    // Stage 8: Multiband OTT
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_ottMix", 1), "Master OTT Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f),
        0.0f)); // skew 0.5: more resolution in subtle blend range
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_ottDepth", 1), "Master OTT Depth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
    // Stage 11: Reverb (FATHOM 8-tap Hadamard FDN — 8 parameters)
    // master_reverbSize and master_reverbMix are the legacy IDs; preserved unchanged.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbSize", 1),
                                                                 "Master Reverb Size",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbMix", 1),
                                                                 "Master Reverb Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_reverbPreDelay", 1), "Master Reverb Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 250.0f, 0.0f, 0.4f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbDecay", 1), "Master Reverb Decay",
                                                    juce::NormalisableRange<float>(0.5f, 20.0f, 0.0f, 0.4f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbDamping", 1),
                                                                 "Master Reverb Damping",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbDiffusion", 1),
                                                                 "Master Reverb Diffusion",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbMod", 1),
                                                                 "Master Reverb Modulation",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_reverbWidth", 1),
                                                                 "Master Reverb Width",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_compRatio", 1), "Master Comp Ratio",
                                                    juce::NormalisableRange<float>(1.0f, 20.0f, 0.0f, 0.4f), 2.5f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_compAttack", 1), "Master Comp Attack",
                                                    juce::NormalisableRange<float>(0.1f, 100.0f, 0.0f, 0.4f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_compRelease", 1), "Master Comp Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 0.0f, 0.4f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_compMix", 1), "Master Comp Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Master FX: Delay parameters (Stage 2)
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_delayTime", 1), "Master Delay Time",
                                                    juce::NormalisableRange<float>(1.0f, 2000.0f, 0.0f, 0.4f), 375.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_delayFeedback", 1),
                                                                 "Master Delay Feedback",
                                                                 juce::NormalisableRange<float>(0.0f, 0.95f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_delayMix", 1), "Master Delay Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_delayPingPong", 1),
                                                                 "Master Delay Ping Pong",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_delayDamping", 1),
                                                                 "Master Delay Damping",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_delayDiffusion", 1),
                                                                 "Master Delay Diffusion",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_delaySync", 1), "Master Delay Sync",
                                                    juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));

    // Master FX: Modulation parameters (Stage 4)
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_modRate", 1), "Master Mod Rate",
                                                    juce::NormalisableRange<float>(0.05f, 10.0f, 0.0f, 0.4f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modDepth", 1), "Master Mod Depth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modMix", 1), "Master Mod Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_modMode", 1), "Master Mod Mode", juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f),
        0.0f)); // 0=Chorus 1=Flanger 2=Ensemble 3=Drift 4=Phaser
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_modFeedback", 1),
                                                                 "Master Mod Feedback",
                                                                 juce::NormalisableRange<float>(0.0f, 0.85f), 0.0f));

    // Master FX: Sequencer parameters (Stage 6)
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqEnabled", 1), "Master Seq Enabled",
                                                    juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqRate", 1), "Master Seq Rate",
                                                    juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 2.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqSteps", 1), "Master Seq Steps",
                                                    juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqDepth", 1), "Master Seq Depth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqSmooth", 1),
                                                                 "Master Seq Smooth",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqTarget1", 1), "Master Seq Target 1",
                                                    juce::NormalisableRange<float>(0.0f, 17.0f, 1.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqTarget2", 1), "Master Seq Target 2",
                                                    juce::NormalisableRange<float>(0.0f, 17.0f, 1.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqPattern", 1), "Master Seq Pattern",
                                                    juce::NormalisableRange<float>(0.0f, 7.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_seqEnvFollow", 1), "Master Seq Env Follow",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_seqEnvAmount", 1),
                                                                 "Master Seq Env Amount",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // Stage 3: Vibe Knob (bipolar: -1 sweet, +1 grit)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_vibeAmount", 1), "Master Vibe", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

    // Stage 4: Spectral Tilt
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_tiltAmount", 1),
                                                                 "Master Tilt Amount",
                                                                 juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_tiltMix", 1), "Master Tilt Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    // Stage 4: Transient Designer
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_tdAttack", 1),
                                                                 "Master TD Attack",
                                                                 juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_tdSustain", 1),
                                                                 "Master TD Sustain",
                                                                 juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_tdMix", 1), "Master TD Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 7: Doppler Effect
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_dopplerDist", 1),
                                                                 "Master Doppler Distance",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_dopplerSpeed", 1),
                                                                 "Master Doppler Speed",
                                                                 juce::NormalisableRange<float>(0.01f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_dopplerMix", 1),
                                                                 "Master Doppler Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 11: Granular Smear
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_smearAmount", 1),
                                                                 "Master Smear Amount",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_smearGrainSize", 1), "Master Smear Grain Size",
        juce::NormalisableRange<float>(10.0f, 200.0f, 0.0f, 0.5f), 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_smearDensity", 1),
                                                                 "Master Smear Density",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_smearMix", 1), "Master Smear Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 12: Harmonic Exciter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_excDrive", 1),
                                                                 "Master Exciter Drive",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_excFreq", 1), "Master Exciter Freq",
        juce::NormalisableRange<float>(1000.0f, 12000.0f, 0.0f, 0.3f), 3500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_excTone", 1),
                                                                 "Master Exciter Tone",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_excMix", 1), "Master Exciter Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 13: Stereo Sculptor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_sculLowWidth", 1),
                                                                 "Master Sculptor Low Width",
                                                                 juce::NormalisableRange<float>(0.0f, 2.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_sculMidWidth", 1),
                                                                 "Master Sculptor Mid Width",
                                                                 juce::NormalisableRange<float>(0.0f, 2.0f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_sculHighWidth", 1),
                                                                 "Master Sculptor High Width",
                                                                 juce::NormalisableRange<float>(0.0f, 2.0f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculLowCross", 1), "Master Sculptor Low Crossover",
        juce::NormalisableRange<float>(60.0f, 500.0f, 0.0f, 0.4f), 200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_sculHighCross", 1), "Master Sculptor High Crossover",
        juce::NormalisableRange<float>(2000.0f, 12000.0f, 0.0f, 0.4f), 4000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_sculMix", 1),
                                                                 "Master Sculptor Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 14: Psychoacoustic Width
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_pwidthAmount", 1),
                                                                 "Master PWidth Amount",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_pwidthHaas", 1), "Master PWidth Haas",
                                                    juce::NormalisableRange<float>(0.1f, 30.0f, 0.0f, 0.4f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_pwidthComb", 1), "Master PWidth Comb Freq",
        juce::NormalisableRange<float>(200.0f, 2000.0f, 0.0f, 0.4f), 600.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_pwidthMono", 1),
                                                                 "Master PWidth Mono Safe",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_pwidthMix", 1),
                                                                 "Master PWidth Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 19.5: Parametric EQ (post-compressor, pre-limiter)
    // Band 1: Low shelf (20–500 Hz, ±12 dB, Q 0.5–5.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB1Freq", 1), "EQ Band 1 Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.0f, 0.35f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB1Gain", 1),
                                                                 "EQ Band 1 Gain",
                                                                 juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB1Q", 1), "EQ Band 1 Q",
                                                                 juce::NormalisableRange<float>(0.5f, 5.0f, 0.0f, 0.4f),
                                                                 0.707f));
    // Band 2: Low-mid peak (100–5000 Hz, ±12 dB, Q 0.5–10.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB2Freq", 1), "EQ Band 2 Freq",
        juce::NormalisableRange<float>(100.0f, 5000.0f, 0.0f, 0.35f), 400.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB2Gain", 1),
                                                                 "EQ Band 2 Gain",
                                                                 juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB2Q", 1), "EQ Band 2 Q",
                                                    juce::NormalisableRange<float>(0.5f, 10.0f, 0.0f, 0.4f), 1.0f));
    // Band 3: High-mid peak (500–15000 Hz, ±12 dB, Q 0.5–10.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB3Freq", 1), "EQ Band 3 Freq",
        juce::NormalisableRange<float>(500.0f, 15000.0f, 0.0f, 0.35f), 3000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB3Gain", 1),
                                                                 "EQ Band 3 Gain",
                                                                 juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB3Q", 1), "EQ Band 3 Q",
                                                    juce::NormalisableRange<float>(0.5f, 10.0f, 0.0f, 0.4f), 1.0f));
    // Band 4: High shelf (2000–20000 Hz, ±12 dB, Q 0.5–5.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_eqB4Freq", 1), "EQ Band 4 Freq",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 0.0f, 0.35f), 10000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB4Gain", 1),
                                                                 "EQ Band 4 Gain",
                                                                 juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_eqB4Q", 1), "EQ Band 4 Q",
                                                                 juce::NormalisableRange<float>(0.5f, 5.0f, 0.0f, 0.4f),
                                                                 0.707f));

    // Stage 20: Brickwall Limiter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_limCeiling", 1),
                                                                 "Master Limiter Ceiling",
                                                                 juce::NormalisableRange<float>(-6.0f, 0.0f), -0.3f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_limRelease", 1), "Master Limiter Release",
                                                    juce::NormalisableRange<float>(10.0f, 500.0f, 0.0f, 0.4f), 50.0f));

    // Stage 6: fXOsmosis (Membrane Transfer)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_osmMembrane", 1),
                                                                 "Master Osmosis Membrane",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_osmReactivity", 1),
                                                                 "Master Osmosis Reactivity",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_osmResonance", 1),
                                                                 "Master Osmosis Resonance",
                                                                 juce::NormalisableRange<float>(0.0f, 0.85f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_osmSaturation", 1),
                                                                 "Master Osmosis Saturation",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_osmMix", 1), "Master Osmosis Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 5.6: fXFormant (Membrane Collection — Formant Filter)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_formantShift", 1), "Formant Shift", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_formantVowel", 1), "Formant Vowel", juce::NormalisableRange<float>(0.0f, 4.0f), 0.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mfx_formantQ", 1), "Formant Resonance",
                                                    juce::NormalisableRange<float>(0.5f, 20.0f, 0.0f, 0.4f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mfx_formantMix", 1), "Formant Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 5.7: fXBreath (Membrane Collection — Breath Texture)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mfx_breathAmount", 1), "Breath Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mfx_breathTilt", 1), "Breath Tilt",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mfx_breathSens", 1),
                                                                 "Breath Sensitivity",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mfx_breathMix", 1), "Breath Mix",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // Stage 12: fXOneiric (Dream State)
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_onDelayTime", 1), "Master Oneiric Delay",
                                                    juce::NormalisableRange<float>(1.0f, 1500.0f, 0.0f, 0.3f), 350.0f));
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_onShiftHz", 1), "Master Oneiric Shift",
                                                    juce::NormalisableRange<float>(-500.0f, 500.0f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_onFeedback", 1),
                                                                 "Master Oneiric Feedback",
                                                                 juce::NormalisableRange<float>(0.0f, 0.92f), 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_onDamping", 1),
                                                                 "Master Oneiric Damping",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("master_onSpread", 1),
                                                                 "Master Oneiric Spread",
                                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("master_onMix", 1), "Master Oneiric Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    // MPE (MIDI Polyphonic Expression) — DAW-automatable per-project settings.
    // Zone layout, pitch-bend range, and expression routing targets are
    // exposed as APVTS parameters so hosts can save/recall them with the project.
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(juce::ParameterID("mpe_enabled", 1), "MPE Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mpe_zone", 1), "MPE Zone", juce::StringArray{"Off", "Lower", "Upper", "Both"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mpe_pitchBendRange", 1), "MPE Pitch Bend Range (semitones)",
        juce::NormalisableRange<float>(1.0f, 96.0f, 1.0f), 48.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mpe_pressureTarget", 1), "MPE Pressure Target",
        juce::StringArray{"Filter Cutoff", "Volume", "Wavetable", "FX Send", "Macro 1 (CHARACTER)",
                          "Macro 2 (MOVEMENT)"},
        0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("mpe_slideTarget", 1), "MPE Slide Target",
        juce::StringArray{"Filter Cutoff", "Volume", "Wavetable", "FX Send", "Macro 1 (CHARACTER)",
                          "Macro 2 (MOVEMENT)"},
        0));

    // ── XOuija Mood Sliders (Wave 5 D2) ─────────────────────────────────────
    // Three global mood parameters that shape heatmap rendering on the XOuija
    // surface.  All are UI-only (no audio path); they are persisted via APVTS
    // so DAW automation and session recall work without extra state machinery.
    //
    //   xouija_brightness — dark ↔ bright  (0=dark, 0.5=neutral, 1=bright)
    //   xouija_tension    — calm ↔ tense   (0=calm, 0.5=neutral, 1=tense)
    //   xouija_density    — sparse ↔ dense (0=sparse, 0.5=neutral, 1=dense)
    //
    // PlaySurface wires onParameterChanged for these three IDs to call
    // xouijaPanel_.setMoodState() so the heatmap repaint happens on the message
    // thread without touching the audio thread.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("xouija_brightness", 1), "XOuija Brightness",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("xouija_tension", 1), "XOuija Tension",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("xouija_density", 1), "XOuija Density",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

    // ── Wave 5 D1: XOuija multi-layer cell parameters ────────────────────────
    // Mood sliders and tendency are automatable APVTS params (per spec section 6).
    // Grid contents (64 cells × 3 layers) are stored in ValueTree only — too many
    // values for APVTS, and editorial state is not automatable.
    // Output routing (one per primary slot) is an int choice param 0-4 (Off…ModSource).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ouija_calm_wild", 1), "XOuija Calm/Wild",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ouija_consonant_dissonant", 1), "XOuija Consonant/Dissonant",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ouija_tendency_col", 1), "XOuija Tendency Col",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ouija_tendency_row", 1), "XOuija Tendency Row",
        juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    for (int s = 0; s < kNumPrimarySlots; ++s)
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID("ouija_route_" + juce::String(s), 1),
            "XOuija Route Slot " + juce::String(s + 1),
            juce::StringArray{ "Off", "Drive Notes", "Drive Sequencer", "Drive Chord", "Mod Source" },
            0 /* default: Off */));

    // AquaticFXSuite::addParameters() uses ParameterLayout::add() (JUCE 7+ API)
    // rather than the shared params vector, so it must be called after constructing
    // the ParameterLayout from the vector.
    juce::AudioProcessorValueTreeState::ParameterLayout layout(params.begin(), params.end());
    xoceanus::AquaticFXSuite::addParameters(layout);
    // EpicChainSlotController registers: slot control params (chain/mix/bypass × 3 slots),
    // Singularity FX params (master_onsl*/master_obs*/master_ora*) which were previously
    // unregistered, and the 4 new Epic chain param sets (onr_/omni_/oblt_/obsc_).
    xoceanus::EpicChainSlotController::addParameters(layout);

    // Wave 5 C1: Per-slot pattern sequencer parameters (primary slots 0–3 only).
    for (int s = 0; s < kNumPrimarySlots; ++s)
        XOceanus::PerEnginePatternSequencer::addParameters(
            layout,
            "slot" + juce::String(s) + "_seq_",
            "Slot " + juce::String(s + 1) + " Seq ");

    // Wave 6: Per-slot play surface layout mode (primary slots 0–3 only).
    // 0 = PlaySurface (KEYS/PADS/XY/OUIJA full window, default)
    // 1 = PadGrid (embedded 4×4 pad grid in engine slot header area)
    // UI-only persistence — no audio-thread reads. Stored in APVTS for
    // DAW session recall. Default = PlaySurface(0).
    for (int s = 0; s < kNumPrimarySlots; ++s)
    {
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID("slot" + juce::String(s) + "_layout_mode", 1),
            "Slot " + juce::String(s + 1) + " Layout",
            juce::StringArray{ "PlaySurface", "PadGrid" },
            0 /* default = PlaySurface */));
    }

    // ── Wave 8: XY Surface parameters (8 params × 4 slots = 32 params) ─────────
    // See Source/UI/PlaySurface/XYSurface.h for full documentation.
    //
    // Per-slot:  xy_pattern, xy_speed, xy_depth, xy_sync,
    //            xy_assignX, xy_assignY, xy_pos_x, xy_pos_y
    //
    // xy_assignX/Y index into the canonical param list:
    //   0=None, 1=FilterCutoff, 2=FilterRes, 3=LFORate, 4=LFODepth,
    //   5=EnvAttack, 6=EnvRelease, 7=Drive, 8=Macro1, 9=Macro2, 10=Macro3,
    //   11=Macro4, 12=FX1WetDry, 13=FX2WetDry, 14=FX3WetDry
    {
        const juce::StringArray kXYPatterns {"None","PULSE","DRIFT","TIDE","RIPPLE","CHAOS"};
        const juce::StringArray kXYSync     {"Free","1bar/4","1bar/2","1bar","2bar","4bar"};
        const juce::StringArray kXYAssign   {
            "None","Filter Cutoff","Filter Res","LFO Rate","LFO Depth",
            "Env Attack","Env Release","Drive",
            "Macro1","Macro2","Macro3","Macro4",
            "FX1 Wet","FX2 Wet","FX3 Wet"
        };
        for (int s = 0; s < kNumPrimarySlots; ++s)
        {
            const juce::String sfx = "_slot" + juce::String(s);
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID("xy_pattern" + sfx, 1),
                "XY Pattern Slot " + juce::String(s + 1), kXYPatterns, 0));
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("xy_speed" + sfx, 1),
                "XY Speed Slot "   + juce::String(s + 1),
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("xy_depth" + sfx, 1),
                "XY Depth Slot "   + juce::String(s + 1),
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID("xy_sync" + sfx, 1),
                "XY Sync Slot "    + juce::String(s + 1), kXYSync, 3)); // default: 1bar
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID("xy_assignX" + sfx, 1),
                "XY Assign X Slot "+ juce::String(s + 1), kXYAssign, 0));
            layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID("xy_assignY" + sfx, 1),
                "XY Assign Y Slot "+ juce::String(s + 1), kXYAssign, 0));
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("xy_pos_x" + sfx, 1),
                "XY Pos X Slot "   + juce::String(s + 1),
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID("xy_pos_y" + sfx, 1),
                "XY Pos Y Slot "   + juce::String(s + 1),
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        }
    }

    return layout;
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
    if (engineId == "Onset" || engineId == "Overbite" || engineId == "OddfeliX" || engineId == "Origami")
        return 100.0f;

    // Reverb-tail / granular / delay — 500ms
    // NOTE: Oxbow (Chiasmus FDN + pre-delay), Overflow (multi-tap delay),
    // Overcast (diffusion cloud), and Oasis (canopy delay) were previously
    // missing from this list and fell through to 200ms, causing premature
    // silence-gating that cut off long reverb tails. Fixed: #764.
    if (engineId == "Overdub" || engineId == "Opal" || engineId == "Oceanic" || engineId == "Obscura" ||
        engineId == "Osprey" || engineId == "Osteria" || engineId == "Ombre" || engineId == "Overlap" ||
        engineId == "Oxbow" || engineId == "Overflow" || engineId == "Overcast" || engineId == "Oasis")
        return 500.0f;

    // Infinite-sustain / self-exciting feedback — 1000ms
    if (engineId == "Organon" || engineId == "Ouroboros" || engineId == "Oracle" || engineId == "Owlfish")
        return 1000.0f;

    // Visual-only — 50ms (Optic generates no audio; gate closes fast)
    if (engineId == "Optic")
        return 50.0f;

    // Standard — 200ms (all remaining engines)
    return 200.0f;
}

void XOceanusProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Guard against non-compliant hosts or test harnesses passing invalid values.
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;
    if (samplesPerBlock <= 0)
        samplesPerBlock = 512;

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

    // EpicChainSlotController: 3-slot FX router (Epic Chains + Wave 2).
    // Runs after masterFX in the signal chain. All slots default to Off
    // (zero-cost passthrough) so existing presets are unaffected until
    // a user selects a chain for a slot.
    epicSlots.prepare(sampleRate, samplesPerBlock);
    epicSlots.cacheParameterPointers(apvts);

    // #1257: Reset MPE channel expression state to match the new sample rate / block size.
    // MPEManager::prepare() calls resetAllChannels() — clears stale per-channel pitch bend
    // and pressure from any previous session so MPE expression starts clean on each
    // prepareToPlay() (host restart, sample rate change, AU validation cycle).
    mpeManager.prepare(sampleRate, samplesPerBlock);

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
            eng->prepareSilenceGate(sampleRate, samplesPerBlock, silenceGateHoldMs(eng->getEngineId()));
            eng->setSharedTransport(&hostTransport);
            eng->setMPEManager(&mpeManager); // issue #1237 — was never called; engines saw nullptr
            // Wave 5 A1: Wire global mod routing pointers for any already-loaded Orrery engine.
            if (auto* orrery = dynamic_cast<OrreryEngine*>(eng.get()))
            {
                orrery->setGlobalCutoffModPtr(&globalCutoffModOffset_);
                orrery->setGlobalLFO1OutPtr(&globalLFO1_);
            }
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
    if (!hasRestoredState.load(std::memory_order_acquire) && std::atomic_load(&engines[0]) == nullptr)
    {
        loadEngine(0, "Obrix");
    }

    // ── Sound on First Launch (§1.1.1 Principle 4) ───────────────────────────
    // If this is TRUE first launch (no restored state AND never launched before),
    // load OXBOW in slot 0 and arm the firstBreathPending_ flag.  processBlock
    // will consume the flag, inject a soft note-on (C3, vel 60), and start the
    // 30-second auto-stop countdown.
    //
    // Guard conditions (all must be true):
    //   1. !hasRestoredState — no DAW session saved state (empty instance)
    //   2. !hasLaunchedBefore_ — never seen the "hasLaunchedBefore" flag in state
    //   3. !firstBreathActive_ — not already playing (re-entrance from auval)
    //   4. !firstBreathPending_ — not already armed this session
    //
    // auval safety: auval calls prepareToPlay repeatedly without setStateInformation.
    // Both guards (no saved state + firstBreathActive_ / firstBreathPending_) ensure
    // we arm at most once per processor lifetime.
    //
    // Preset parameter application: we apply "Oxbow_Breath_Mist" parameters inline
    // (no disk I/O) using APVTS setValueNotifyingHost(), which is message-thread-safe.
    // Parameters match Presets/XOceanus/Organic/Oxbow/Oxbow_Breath_Mist.xometa verbatim.
    if (!hasRestoredState.load(std::memory_order_acquire) &&
        !hasLaunchedBefore_.load(std::memory_order_acquire) &&
        !firstBreathActive_ &&
        !firstBreathPending_.load(std::memory_order_relaxed))
    {
        // Swap slot 0 from Obrix → Oxbow for the First Breath experience.
        // This replaces the just-loaded Obrix (above) with the atmospheric pad engine.
        loadEngine(0, "Oxbow");

        // Apply "Breath Mist" Oxbow parameters inline — no disk I/O required.
        // Values taken verbatim from Oxbow_Breath_Mist.xometa (Organic mood).
        // Uses setValueNotifyingHost so the UI reflects the initial state.
        // Parameters are normalized via convertTo0to1 to respect APVTS range contracts.
        struct BreathMistParam { const char* id; float value; };
        static const BreathMistParam kBreathMistParams[] = {
            {"oxb_size",         0.1f},
            {"oxb_decay",        0.5f},
            {"oxb_entangle",     0.06f},
            {"oxb_erosionRate",  0.05f},
            {"oxb_erosionDepth", 0.08f},
            {"oxb_convergence",  4.0f},
            {"oxb_resonanceQ",   3.5f},
            {"oxb_resonanceMix", 0.15f},
            {"oxb_cantilever",   0.12f},
            {"oxb_damping",      7000.0f},
            {"oxb_predelay",     0.0f},
            {"oxb_dryWet",       0.15f},
            {"oxb_exciterDecay", 0.05f},
            {"oxb_exciterBright",0.5f},
        };
        for (const auto& p : kBreathMistParams)
        {
            if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(p.id)))
                param->setValueNotifyingHost(param->convertTo0to1(p.value));
        }

        firstBreathPending_.store(true, std::memory_order_release);
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

    // Wave 5 C1: prepare per-slot pattern sequencers
    for (auto& seq : slotSequencers_)
        seq.prepareToPlay(sampleRate);

    // Wave 5 D1: prepare XOuija walk engine
    ouijaWalkEngine_.prepareToPlay(sampleRate, samplesPerBlock);
}

void XOceanusProcessor::releaseResources()
{
    for (int i = 0; i < MaxSlots; ++i)
    {
        auto eng = std::atomic_load(&engines[i]);
        if (eng)
            eng->releaseResources();
    }
    masterFX.reset();
    epicSlots.reset();
}

void XOceanusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    xoceanus::dsp::initAudioThreadOnce(); // ARM64 FPCR.FZ — see ThreadInit.h
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // CPU load measurement: record start time (high-res ticks)
    processBlockStartTick = juce::Time::getHighResolutionTicks();

    // Consume kill-delay-tails request — must run before any audio processing
    // so the reset takes effect before any FX contributes this block.
    if (killDelayTailsPending.load(std::memory_order_acquire))
    {
        masterFX.reset();
        epicSlots.reset();
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

    // Phase 0 wildcard infrastructure: drive the DNAModulationBus from M1 CHARACTER.
    // M1 is a 0..1 macro; we pass it verbatim so DNA is warped only toward the
    // "fully characterful" end. Future bipolar mapping (-1..1) would let M1 also
    // pull DNA toward its inverse, but Phase 0 keeps the convention simple.
    // applyMacroWarp + advanceSmoothing are both lock-free; safe on the audio thread.
    if (auto* m1Ptr = apvts.getRawParameterValue("macro1"))
    {
        const float m1 = m1Ptr->load(std::memory_order_relaxed);
        for (int slot = 0; slot < xoceanus::DNAModulationBus::MaxEngineSlots; ++slot)
            dnaBus_.applyMacroWarp(slot, m1);
    }
    dnaBus_.advanceSmoothing(numSamples);

    // Build engine pointer array for coupling matrix (atomic reads)
    std::array<SynthEngine*, MaxSlots> enginePtrs = {};
    std::array<std::shared_ptr<SynthEngine>, MaxSlots> engineRefs; // prevent deletion during block
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
            crossfades[i].outgoing = std::move(pending.outgoing);
            crossfades[i].fadeGain = pending.fadeGain;
            crossfades[i].fadeSamplesRemaining = pending.fadeSamplesRemaining;
            crossfades[i].needsAllNotesOff = pending.needsAllNotesOff;
            // Release the slot so the message thread may post another swap.
            pending.ready.store(false, std::memory_order_release);
        }
    }

    // Drain PlaySurface MIDI before early return to prevent unbounded queue growth
    playSurfaceMidiCollector.removeNextBlockOfMessages(midi, numSamples);

    if (activeCount == 0)
        return;

    couplingMatrix.setEngines(enginePtrs);

    // Merge PlaySurface note-on/off events queued from the message thread.
    // removeNextBlockOfMessages() is thread-safe; it drains the lock-free queue
    // and appends the messages into `midi` so the rest of the pipeline sees them
    // exactly like host-generated MIDI events. (Drained above, before early return.)

    // Process MIDI learn CC → parameter routing (audio thread safe)
    midiLearnManager.processMidi(midi);

    // Sync Chord Machine state from cached parameter pointers (no hash lookups)
    chordMachine.setEnabled(cachedParams.cmEnabled->load() >= 0.5f);
    chordMachine.setPalette(static_cast<PaletteType>(static_cast<int>(cachedParams.cmPalette->load())));
    chordMachine.setVoicing(static_cast<VoicingMode>(static_cast<int>(cachedParams.cmVoicing->load())));
    chordMachine.setSpread(cachedParams.cmSpread->load());
    chordMachine.setSequencerRunning(cachedParams.cmSeqRunning->load() >= 0.5f);
    chordMachine.setBPM(cachedParams.cmSeqBpm->load());
    chordMachine.setSwing(cachedParams.cmSeqSwing->load());
    chordMachine.setGlobalGate(cachedParams.cmSeqGate->load());
    chordMachine.setVelocityCurve(static_cast<VelocityCurve>(static_cast<int>(cachedParams.cmVelCurve->load())));
    chordMachine.setHumanize(cachedParams.cmHumanize->load());
    chordMachine.setSidechainDuck(cachedParams.cmSidechainDuck->load());
    chordMachine.setEnoMode(cachedParams.cmEnoMode->load() >= 0.5f);
    // Wave 5 B3: sync per-slot chord/seq routing modes from cached APVTS params.
    for (int slot = 0; slot < 4; ++slot)
    {
        if (cachedParams.cmSlotRoute[slot] != nullptr)
        {
            const int modeIdx = static_cast<int>(cachedParams.cmSlotRoute[slot]->load(std::memory_order_relaxed));
            chordMachine.setSlotRoutingMode(slot, static_cast<ChordSeqRoutingMode>(
                std::max(0, std::min(modeIdx, static_cast<int>(ChordSeqRoutingMode::NumModes) - 1))));
        }
    }
    // W12 fix: sync pattern from APVTS on every block.  applyPattern() is safe to
    // call from the audio thread — it writes to steps[].active (benign race as
    // documented in ChordMachine.h) and updates activePattern atomic.
    // We only call applyPattern when the pattern index changes to avoid clobbering
    // per-step edits the user may have made (Eno mutations, UI step overrides).
    {
        const auto newPat = static_cast<RhythmPattern>(static_cast<int>(cachedParams.cmSeqPattern->load()));
        if (newPat != chordMachine.getPattern())
            chordMachine.applyPattern(newPat);
    }

    // ── B2: Sync chord input mode + global key/scale + pad chord slots ─────────
    if (cachedParams.cmInputMode)
        chordMachine.setInputMode(static_cast<ChordInputMode>(
            static_cast<int>(cachedParams.cmInputMode->load())));
    if (cachedParams.cmGlobalRoot)
        chordMachine.setGlobalRootKey(static_cast<int>(cachedParams.cmGlobalRoot->load()));
    if (cachedParams.cmGlobalScale)
        chordMachine.setGlobalScaleIndex(static_cast<int>(cachedParams.cmGlobalScale->load()));
    // Sync 16 pad chord slots from APVTS (block-constant snapshot, no allocation)
    for (int i = 0; i < 16; ++i)
    {
        if (cachedParams.padChords[i].root != nullptr)
        {
            chordMachine.setPadChord(
                i,
                static_cast<int>(cachedParams.padChords[i].root->load()),
                static_cast<VoicingMode>(static_cast<int>(cachedParams.padChords[i].voicing->load())),
                static_cast<int>(cachedParams.padChords[i].inv->load()));
        }
    }

    // Sync MPE manager from cached APVTS parameters (no hash lookups)
    if (cachedParams.mpeEnabled)
    {
        mpeManager.setMPEEnabled(cachedParams.mpeEnabled->load() >= 0.5f);
        mpeManager.setZoneLayout(static_cast<MPEZoneLayout>(static_cast<int>(cachedParams.mpeZone->load())));
        mpeManager.setPitchBendRange(static_cast<int>(cachedParams.mpePitchBendRange->load()));
        mpeManager.setPressureTarget(
            static_cast<MPEManager::ExpressionTarget>(static_cast<int>(cachedParams.mpePressureTarget->load())));
        mpeManager.setSlideTarget(
            static_cast<MPEManager::ExpressionTarget>(static_cast<int>(cachedParams.mpeSlideTarget->load())));
    }

    // ── MPE expression extraction (#1237 — was dead; now wired) ──────────────
    // Parse pitch-bend, channel-pressure, aftertouch, and CC74 from the raw
    // MIDI stream into per-channel expression state so MPE-aware engines can
    // call mpeManager->updateVoiceExpression() in their renderBlock().
    // mpeMidiBuffer receives the expression-stripped MIDI (pitch wheel removed,
    // note-on/off and all other messages pass through).
    // Raw `midi` is still forwarded to ChordMachine below so the ~85 non-MPE-aware
    // engines that parse pitch wheel directly from their MIDI stream continue to
    // function — per-channel expression is available via mpeExpression for the
    // engines that have been upgraded to use it (#1237).
    mpeManager.processBlock(midi, mpeMidiBuffer); // issue #1237 — was never called

    // ── External MIDI Clock sync (#359) ──────────────────────────────────────
    // Scan incoming MIDI for system real-time messages (0xF8 Clock, 0xFA Start,
    // 0xFB Continue, 0xFC Stop).  These are single-byte messages that arrive
    // interleaved with note data and do not carry a channel.
    //
    // BPM derivation:  MIDI Clock fires 24 times per quarter note.  Six pulses
    // equal one 16th note.  We measure the elapsed samples between consecutive
    // step boundaries and compute:
    //   BPM = (sampleRate * 60) / (intervalSamples * 4)
    // A one-pole smoothing filter (α ≈ 0.3) reduces jitter without adding
    // significant latency.
    {
        for (const auto& meta : midi)
        {
            const auto& msg = meta.getMessage();
            const int spos = meta.samplePosition;
            const double absoluteTime = midiClockBlockOffset_ + spos;

            if (msg.isMidiClock()) // 0xF8
            {
                if (chordMachine.receiveMidiClockPulse())
                {
                    // Step boundary: derive BPM from elapsed samples since last step
                    if (midiClockLastStepTime_ >= 0.0)
                    {
                        const double intervalSamples = absoluteTime - midiClockLastStepTime_;
                        if (intervalSamples > 0.0)
                        {
                            // intervalSamples = samples per 16th note
                            // BPM = (SR * 60) / (intervalSamples * 4)
                            const float rawBPM = static_cast<float>(getSampleRate() * 60.0 / (intervalSamples * 4.0));
                            // Clamp to sensible range before smoothing
                            const float clampedBPM = juce::jlimit(20.0f, 300.0f, rawBPM);
                            // One-pole smoothing (α ≈ 0.3 — fast enough for live feel)
                            midiClockDerivedBPM_ = 0.7f * midiClockDerivedBPM_ + 0.3f * clampedBPM;
                        }
                    }
                    midiClockLastStepTime_ = absoluteTime;
                    chordMachine.advanceExternalClockStep(midiClockDerivedBPM_);
                }
            }
            else if (msg.isMidiStart()) // 0xFA
            {
                midiClockLastStepTime_ = absoluteTime;
                chordMachine.receiveMidiStart();
            }
            else if (msg.isMidiContinue()) // 0xFB
            {
                chordMachine.receiveMidiContinue();
            }
            else if (msg.isMidiStop()) // 0xFC
            {
                chordMachine.receiveMidiStop();
            }
        }
        // Advance the running sample-time accumulator by this block's length.
        // Done after scanning so spos values within the block are relative to
        // midiClockBlockOffset_ (i.e. absoluteTime = offset + spos).
        midiClockBlockOffset_ += numSamples;
    }

    // DAW host transport sync — read PlayHead once, feed both hostTransport
    // (consumed by tempo-synced engines via SharedTransport) and ChordMachine.
    {
        juce::AudioPlayHead::PositionInfo posSnapshot;
        const juce::AudioPlayHead::PositionInfo* posPtr = nullptr;
        if (auto* playHead = getPlayHead())
        {
            if (auto pos = playHead->getPosition())
            {
                posSnapshot = *pos;
                posPtr = &posSnapshot;
            }
        }
        hostTransport.processBlock(numSamples,
                                   currentSampleRate.load(std::memory_order_relaxed),
                                   posPtr);
        if (posPtr != nullptr && posPtr->getPpqPosition().hasValue())
        {
            const double ppq = *posPtr->getPpqPosition();
            double hostBPM = 122.0;
            if (posPtr->getBpm().hasValue())
                hostBPM = *posPtr->getBpm();
            const bool hostPlaying = posPtr->getIsPlaying();
            chordMachine.syncToHost(ppq, hostBPM, hostPlaying);
        }
    }

    // Route MIDI through ChordMachine → 4 per-slot MidiBuffers.
    // When disabled, each slot gets a copy of the input MIDI (previous behavior).
    // When enabled, each slot gets its own chord-distributed note.
    // NOTE (#1237): We pass raw `midi` (not mpeMidiBuffer) here so that the ~85
    // non-MPE-aware engines that parse pitch-wheel from their MIDI stream continue
    // to function correctly. MPE-aware engines (Opal, Oblong, Overbite, Orca,
    // Octopus, Ouie) use mpeExpression.pitchBendSemitones populated above.
    // Double-apply risk for MPE-aware engines in non-MPE mode (P29 / Bob) is a
    // pre-existing issue flagged for separate fix — do not conflate with #1237.
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
                const int ch = msg.getChannel() - 1; // 0-based
                expressionValue_[ch] = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }

            if (msg.isControllerOfType(64))
            {
                const int ch = msg.getChannel() - 1; // 0-based
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
                                slotMidi[slot].addEvent(juce::MidiMessage::noteOff(ch + 1, note, (uint8_t)0), 0);
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
                    const int ch = msg.getChannel() - 1;
                    const int note = msg.getNoteNumber();
                    if (sustainHeld_[ch])
                    {
                        // Defer: remember this note needs a release later.
                        sustainPendingNoteOffs_[slot][ch].set(note);
                        continue; // suppress the note-off for now
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
                slotMidi[s].addEvent(juce::MidiMessage::noteOn(1, note, vel), 0);
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
                slotMidi[s].addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
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
                    if (note > 0 && note <= 127) // 0 = sentinel (empty); MIDI note C-1 excluded
                        slotMidi[s].addEvent(juce::MidiMessage::noteOff(1, note), 0);
                }
            }
            else
            {
                chordFireNoteOffCountdown.store(newCountdown, std::memory_order_relaxed);
            }
        }
    }

    // ── Sound on First Launch — §1.1.1 Principle 4 ───────────────────────────
    // firstBreathPending_ is set by prepareToPlay() exactly once on TRUE first launch.
    // We consume it here (audio thread) and inject a sustained soft C3 note into
    // slot 0.  The note lives until:
    //   a) any real MIDI note-on arrives from host/PlaySurface (user plays), or
    //   b) the 30-second failsafe countdown reaches zero.
    // Both conditions send a note-off and clear firstBreathActive_.
    //
    // Audio-thread-only after consumption — no atomics needed for Active/Countdown.
    {
        // Arm: consume the pending flag and inject the initial note-on.
        if (firstBreathPending_.load(std::memory_order_acquire))
        {
            firstBreathPending_.store(false, std::memory_order_release);
            if (enginePtrs[0] != nullptr) // engine must be ready
            {
                firstBreathActive_         = true;
                firstBreathFading_         = false; // reset any in-progress fade from a prior replay
                firstBreathFadeCountdown_  = 0;
                firstBreathGeneration_     = engineGeneration_.load(std::memory_order_acquire); // snapshot generation at arm time
                firstBreathCountdown_      = static_cast<int>(
                    currentSampleRate.load(std::memory_order_relaxed) * kFirstBreathTimeoutMs / 1000.0);
                slotMidi[0].addEvent(
                    juce::MidiMessage::noteOn(1, kFirstBreathNote, kFirstBreathVelocity), 0);
            }
        }

        if (firstBreathActive_)
        {
            // Engine-change cancellation: if the engine generation counter has advanced
            // since arm time, the slot-0 engine has been swapped (loaded or unloaded).
            // Comparing raw pointers to a potentially-freed object is UB (#756); the
            // generation counter is safe because it is an atomic integer that is never
            // freed.  The outgoing engine gets AllNotesOff via the 50ms crossfade
            // mechanism, so we simply disarm without an extra note-off here.
            if (engineGeneration_.load(std::memory_order_relaxed) != firstBreathGeneration_)
            {
                firstBreathActive_        = false;
                firstBreathFading_        = false;
                firstBreathFadeCountdown_ = 0;
                firstBreathCountdown_     = 0;
            }
            else
            {
                // Check whether any real MIDI note-on arrived this block from host or PlaySurface.
                // We scan the raw `midi` buffer (pre-ChordMachine) rather than slotMidi so we
                // catch genuine performer input.  Mouse clicks on the PlaySurface also arrive
                // here via playSurfaceMidiCollector (drained into `midi` above).
                bool userPlayed = false;
                for (const auto& meta : midi)
                {
                    const auto& msg = meta.getMessage();
                    if (msg.isNoteOn() && msg.getVelocity() > 0)
                    {
                        userPlayed = true;
                        break;
                    }
                }

                if (userPlayed && !firstBreathFading_)
                {
                    // §1300: user interaction — start the 200 ms fade window instead of
                    // killing the note immediately.  The note-off fires after the fade
                    // expires so there is no abrupt cut when the user first plays.
                    firstBreathFading_         = true;
                    firstBreathFadeCountdown_  = static_cast<int>(
                        currentSampleRate.load(std::memory_order_relaxed) * kFirstBreathFadeMs / 1000.0);
                }

                if (firstBreathFading_)
                {
                    firstBreathFadeCountdown_ -= numSamples;
                    if (firstBreathFadeCountdown_ <= 0)
                    {
                        // Fade complete — send note-off and disarm.
                        slotMidi[0].addEvent(juce::MidiMessage::noteOff(1, kFirstBreathNote, (uint8_t)0), 0);
                        firstBreathActive_         = false;
                        firstBreathFading_         = false;
                        firstBreathCountdown_      = 0;
                        firstBreathFadeCountdown_  = 0;
                    }
                }
                else
                {
                    // Decrement 30-second failsafe countdown.
                    firstBreathCountdown_ -= numSamples;
                    if (firstBreathCountdown_ <= 0)
                    {
                        slotMidi[0].addEvent(juce::MidiMessage::noteOff(1, kFirstBreathNote, (uint8_t)0), 0);
                        firstBreathActive_    = false;
                        firstBreathCountdown_ = 0;
                    }
                }
            }
        }
    }
    // ── end Sound on First Launch ─────────────────────────────────────────────

    // ── Wave 5 C1: Per-slot pattern sequencers ────────────────────────────────
    // Run AFTER all slotMidi[] population (ChordMachine, sustain, firstBreath)
    // and BEFORE engine renderBlock so sequencer events are processed this block.
    // Reuse the transport values already read into hostTransport this block.
    {
        // I4: Pre-computed slot APVTS prefix strings — avoids juce::String allocation
        // every block.  "slot0_seq_" … "slot3_seq_" match the IDs registered in
        // createParameterLayout via PerEnginePatternSequencer::addParameters().
        static const juce::String kSlotSeqPrefix[kNumPrimarySlots] = {
            "slot0_seq_", "slot1_seq_", "slot2_seq_", "slot3_seq_"
        };

        const double seqBpm      = hostTransport.getBPM();
        const double seqPpq      = hostTransport.getBeatPosition();
        const bool   seqPlaying  = hostTransport.isPlaying();
        for (int s = 0; s < kNumPrimarySlots; ++s)
        {
            slotSequencers_[s].syncFromApvts(apvts, kSlotSeqPrefix[s]);
            // C3: sync per-step gate override + pitch offset from APVTS params.
            slotSequencers_[s].syncStepOverridesFromApvts(apvts, kSlotSeqPrefix[s]);
            slotSequencers_[s].processBlock(slotMidi[s], seqBpm, seqPpq, seqPlaying, numSamples);
        }
    }
    // ── end Wave 5 C1 ────────────────────────────────────────────────────────

    // ── Wave 5 D1: XOuija walk engine ─────────────────────────────────────────
    // Push mood/tendency atomics from APVTS cached pointers (no string lookups),
    // then advance the planchette.  Phase B output router (DriveNotes/DriveSeq/
    // DriveChord/ModSource) will be wired here once those downstream systems land.
    {
        if (cachedParams.ouijaCalmWild)
            ouijaWalkEngine_.setCalmWild(cachedParams.ouijaCalmWild->load(std::memory_order_relaxed));
        if (cachedParams.ouijaConsonantDissonant)
            ouijaWalkEngine_.setConsonantDissonant(cachedParams.ouijaConsonantDissonant->load(std::memory_order_relaxed));
        if (cachedParams.ouijaTendencyCol)
            ouijaWalkEngine_.setTendencyCol(cachedParams.ouijaTendencyCol->load(std::memory_order_relaxed));
        if (cachedParams.ouijaTendencyRow)
            ouijaWalkEngine_.setTendencyRow(cachedParams.ouijaTendencyRow->load(std::memory_order_relaxed));

        ouijaWalkEngine_.processBlock(numSamples,
                                      hostTransport.getBPM(),
                                      hostTransport.getBeatPosition(),
                                      hostTransport.isPlaying());
    }
    // ── end Wave 5 D1 ─────────────────────────────────────────────────────────

    // Feed external audio to Osmosis if loaded in any slot.
    // Uses virtual isAnalysisEngine() instead of dynamic_cast to avoid RTTI on audio thread.
    for (int slot = 0; slot < MaxSlots; ++slot)
    {
        if (enginePtrs[slot] && enginePtrs[slot]->isAnalysisEngine())
        {
            static_cast<OsmosisEngine*>(enginePtrs[slot])
                ->setExternalInput(externalInputBuffer.getReadPointer(0), externalInputBuffer.getReadPointer(1),
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
                    if (!gateWoken)
                    {
                        enginePtrs[i]->wakeSilenceGate();
                        gateWoken = true;
                    }
                    // Push to Field Map queue (lock-free, drops if full — no block)
                    const auto& msg = metadata.getMessage();
                    pushNoteEvent(msg.getNoteNumber(), msg.getFloatVelocity(), i);
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
            EngineProfiler::ScopedMeasurement measurement(engineProfilers[i]);
            enginePtrs[i]->renderBlock(engineBuffers[i], slotMidi[i], numSamples);
        }

        // SRO: Feed output to silence gate for analysis
        enginePtrs[i]->analyzeForSilenceGate(engineBuffers[i], numSamples);

        // SRO: Record slot stats for auditor (CPU + silence gate state)
        sroAuditor.recordSlot(i, engineProfilers[i].getStats(), enginePtrs[i]->isSilenceGateBypassed());

        // Waveform FIFO push — capture raw engine output (pre-coupling, pre-master FX)
        // for the UI oscilloscope.  No allocation; O(n) copy is fine at 512 samples.
        waveformFifos[i].push(engineBuffers[i].getReadPointer(0), static_cast<size_t>(numSamples));
    }

    // ── Wave 5 A1: Evaluate global mod routes ────────────────────────────────
    // Read the snapshot version.  If it changed since our last consume, the
    // snapshot array is up to date (message thread wrote it with release semantics).
    // We then iterate the fixed-size array and apply each route.
    //
    // RT-safe:
    //   • No allocation.  All data lives in pre-allocated members.
    //   • Route iteration uses routesSnapshotCount_ written on the message thread
    //     before the version increment; the acquire fence ensures we see the count.
    //   • Destination parameter writes use atomic store (relaxed) on the APVTS
    //     parameter's raw float.  APVTS's own parameter system does the same on
    //     host automation.
    //
    // For each route we:
    //   1. Identify the source value (LFO1 only in A1; expanded in A2).
    //   2. Compute modulated offset = sourceValue * depth.
    //   3. Add offset to the current parameter value (read + write via atomic).
    //
    // Bipolar check: use != 0 per CLAUDE.md (negative depth sweeps downward).
    {
        const int curVer = snapshotVersion_.load(std::memory_order_relaxed);
        if (curVer != audioSnapshotVersion_)
        {
            // Acquire fence: synchronizes with the release fence in flushModRoutesSnapshot().
            // Guarantees all snapshot array writes are visible before we iterate.
            std::atomic_thread_fence(std::memory_order_acquire);
            audioSnapshotVersion_ = curVer;
        }

        const int nRoutes = routesSnapshotCount_.load(std::memory_order_relaxed);
        if (nRoutes > 0)
        {
            // Gather source values (audio-thread sources).
            // In A1 we have LFO1 only; A2 will add LFO2, Envelope, etc.
            const float lfo1Val = globalLFO1_.load(std::memory_order_relaxed);

            // B1: Zero per-route accumulators before evaluating this block.
            // Plain floats — all access is on the audio thread, no atomics needed.
            routeModAccum_.fill(0.0f);

            // Accumulate global cutoff mod offset (same units as OrreryEngine::modCutoffOffset).
            float globalCutoffMod = 0.0f;

            for (int ri = 0; ri < nRoutes; ++ri)
            {
                const auto& snap = routesSnapshot_[static_cast<size_t>(ri)];
                if (!snap.valid)
                    continue;

                // Source value — only LFO1 (id=0) wired in A1.
                // C5: SeqStepValue / BeatPhase / LiveGate read from slotSequencers_.
                // #1289: SeqStepPitch added — per-step pitch offset as bipolar -1..+1.
                // TODO(#mod-source-completion): implement LFO2, LFO3, Envelope, Envelope2,
                //   Velocity, Aftertouch, ModWheel, MacroTone/Tide/Couple/Depth, MidiCC,
                //   MpePressure, MpeSlide, XouijaCell (each needs separate scoping work).
                float srcVal = 0.0f;
                if (snap.sourceId == static_cast<int>(ModSourceId::LFO1))
                {
                    srcVal = lfo1Val;
                }
                else if (snap.sourceId == static_cast<int>(ModSourceId::SeqStepValue) ||
                         snap.sourceId == static_cast<int>(ModSourceId::BeatPhase)    ||
                         snap.sourceId == static_cast<int>(ModSourceId::LiveGate)     ||
                         snap.sourceId == static_cast<int>(ModSourceId::SeqStepPitch))
                {
                    // Validate slot index — guard against stale or bad snapshots.
                    const int slot = snap.slotIndex;
                    if (slot < 0 || slot >= kNumPrimarySlots)
                        continue; // no slot assigned — skip route

                    if (snap.sourceId == static_cast<int>(ModSourceId::SeqStepValue))
                    {
                        // Velocity 0.0–1.0 (unipolar from the sequencer's gate).
                        // Already 0 when the step is silent, so bipolar flag maps to ±.
                        srcVal = slotSequencers_[static_cast<size_t>(slot)].getLiveVelocity();
                    }
                    else if (snap.sourceId == static_cast<int>(ModSourceId::BeatPhase))
                    {
                        // Step phase 0.0–1.0 — expose as bipolar -1..+1 by mapping 0..1 → -1..+1.
                        const float phase = slotSequencers_[static_cast<size_t>(slot)].getLiveStepPhase();
                        srcVal = phase * 2.0f - 1.0f; // bipolar ramp
                    }
                    else if (snap.sourceId == static_cast<int>(ModSourceId::SeqStepPitch))
                    {
                        // #1289: per-step pitch offset, normalised to -1..+1 from ±12 semitones.
                        // 0.0 on silent (rest) steps so mod depth does not ghost-ring.
                        srcVal = slotSequencers_[static_cast<size_t>(slot)].getLiveStepPitch();
                    }
                    else // LiveGate — gate state (0 or 1, unipolar)
                    {
                        srcVal = slotSequencers_[static_cast<size_t>(slot)].getLiveGate();
                    }
                }
                else
                    continue; // TODO(#mod-source-completion): add remaining sources

                // Bipolar: use != 0 check so negative depths sweep downward.
                if (snap.depth == 0.0f)
                    continue;

                const float modOffset = srcVal * snap.depth;

                // B1: Write per-route accumulator — available to any engine that opts in
                // via getModRouteAccum(ri).  Units: normalised modOffset in [-depth, +depth].
                routeModAccum_[static_cast<size_t>(ri)] = modOffset;

                // B1: Destination dispatch — use pre-resolved pointer flags, no strncmp.
                if (snap.isOrryCutoff)
                {
                    // Orrery filter cutoff: scale by 8000 Hz to match OrreryEngine's mod
                    // matrix convention (dest[1] * 8000 = cutoff offset in Hz).
                    globalCutoffMod += modOffset * 8000.0f;
                }
                // else: destination is available via routeModAccum_[ri] for engines to read.
                // When an engine opt-in API is added (Phase A2+), it will call
                // getModRouteAccum(ri) with the route index it cached at load time.
                // No silent discard — the value is now correctly stored in routeModAccum_.
            }

            // Write accumulated cutoff offset for OrreryEngine to read.
            // Audio thread: relaxed ordering — engine reads this in the same processBlock
            // call (always on the audio thread, so no cross-thread ordering required).
            globalCutoffModOffset_.store(globalCutoffMod, std::memory_order_relaxed);
        }
        else
        {
            // No routes — ensure offset is zero so filter settles to base value.
            globalCutoffModOffset_.store(0.0f, std::memory_order_relaxed);
            routeModAccum_.fill(0.0f);
        }
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
                const int rawType =
                    juce::jlimit(0, static_cast<int>(CouplingType::KnotTopology), juce::roundToInt(cp.type->load()));
                const auto perfType = static_cast<CouplingType>(rawType);

                const float perfAmount = cp.amount->load();
                const int perfSource = static_cast<int>(cp.source->load());
                const int perfTarget = static_cast<int>(cp.target->load());

                // Bounds-check slot indices to prevent OOB
                if (perfSource < 0 || perfSource >= MaxSlots || perfTarget < 0 || perfTarget >= MaxSlots ||
                    perfSource == perfTarget)
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
                perfRoute.sourceSlot = perfSource;
                perfRoute.destSlot = perfTarget;
                perfRoute.type = perfType;
                perfRoute.amount = perfAmount;
                perfRoute.isNormalled = false;
                perfRoute.active = true;

                // Override or append: if a baseline route targets the same
                // source->dest pair, the performance route wins.
                bool overridden = false;
                for (auto& baseRoute : merged)
                {
                    if (baseRoute.sourceSlot == perfSource && baseRoute.destSlot == perfTarget)
                    {
                        baseRoute.type = perfRoute.type;
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
                const int totalBurstSamples =
                    static_cast<int>(currentSampleRate.load(std::memory_order_relaxed) * kCouplingBurstMs / 1000.0);
                const float progress =
                    1.0f - static_cast<float>(newRemaining) / static_cast<float>(std::max(1, totalBurstSamples));
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

    // Apply BROTH quad multi-timescale diffusion coupling
    // (OVERWASH / OVERWORN / OVERFLOW / OVERCAST — Kitchen Collection Pads)
    processBrothCoupling(enginePtrs);

    // Mix all engine outputs to master — skip muted slots.
    const float masterVol = cachedParams.masterVolume->load();

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;

        // Respect per-slot mute state (written by message thread via setSlotMuted)
        if (slotMuted[i].load(std::memory_order_relaxed))
            continue;

        // Process engine output through its insert shaper (if loaded).
        // Uses the same shared_ptr-copy pattern as bus shapers in MasterFXChain.
        {
            juce::MidiBuffer emptyMidi;
            ShaperRegistry::instance().processInsert(i, engineBuffers[i], emptyMidi, numSamples);
        }

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
            // Move the outgoing engine to the graveyard rather than calling
            // shared_ptr::reset() here.  reset() would trigger ~SynthEngine()
            // with 20+ vector deallocations on the RT thread — a hard real-time
            // violation.  drainGraveyard() is dispatched to the message thread
            // via callAsync (below) so destruction happens safely off the RT path.
            if (cf.outgoing)
                depositInGraveyard(std::move(cf.outgoing));
            cf.fadeGain = 0.0f;
        }
    }

    // Drain the graveyard on the message thread if anything was deposited this block.
    // Using MessageManager::callAsync (not juce::Timer) — some iOS AUv3 hosts run
    // Timer callbacks on the audio thread, which would defeat the purpose.
    if (graveyardWritePos_.load(std::memory_order_relaxed) !=
        graveyardReadPos_.load(std::memory_order_relaxed))
    {
        juce::MessageManager::callAsync([this] { drainGraveyard(); });
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
    // EpicChainSlotController: 3 assignable FX slots (each holds one of 30 chains
    // or Off). Runs post-master FX. Note argument order: (buffer, numSamples,
    // bpm, ppqPos) — differs from masterFX's (buffer, numSamples, ppqPos, bpm).
    epicSlots.processBlock(buffer, numSamples, bpm, ppqPos);
    masterOutputFifo.push(buffer.getReadPointer(0), static_cast<size_t>(numSamples));

    // NaN/inf guard — engine bugs should not crash the host
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            if (!std::isfinite(data[i]))
                data[i] = 0.0f;
    }

    // CPU load measurement: elapsed / buffer_duration, smoothed with a leaky integrator.
    // Uses high-resolution ticks so measurements are host-independent.
    const double sr_ = currentSampleRate.load(std::memory_order_relaxed);
    if (sr_ > 0.0 && numSamples > 0)
    {
        const juce::int64 endTick = juce::Time::getHighResolutionTicks();
        const double ticksPerSec = static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());
        const double elapsedSec = static_cast<double>(endTick - processBlockStartTick) / ticksPerSec;
        const double bufferDurationSec = static_cast<double>(numSamples) / sr_;
        const float rawLoad = static_cast<float>(elapsedSec / bufferDurationSec);
        // Leaky integrator: 90% previous + 10% new — smooths out single-block spikes.
        const float prevLoad = processingLoad.load(std::memory_order_relaxed);
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
        float coeff = (target > current) ? 0.3f : 0.05f;   // fast attack, slow release
        noteActivity_.store(current + coeff * (target - current), std::memory_order_relaxed);
    }

    // Emit any pending CC output events from the XOuija UI (lock-free SPSC drain).
    drainCCOutput(midi, numSamples);
}

void XOceanusProcessor::processFamilyBleed(std::array<SynthEngine*, MaxSlots>& enginePtrs)
{
    // Identify which slots hold Constellation family engines
    static const juce::StringArray kFamilyIds = {"Ohm", "Orphica", "Obbligato", "Ottoni", "Ole"};

    // Collect family engine pointers and their slot indices.
    // familySlots_ is a member pre-allocated in prepareToPlay — no heap alloc here.
    familySlots_.clearQuick();

    for (int i = 0; i < MaxSlots; ++i)
    {
        if (!enginePtrs[i])
            continue;
        if (kFamilyIds.contains(enginePtrs[i]->getEngineId()))
            familySlots_.add({i, enginePtrs[i]});
    }

    if (familySlots_.size() < 2)
        return; // nothing to bleed

    // Read macro values from cached parameter pointers — safe on audio thread
    const float communeAmt = cachedParams.ohmCommune ? cachedParams.ohmCommune->load() : 0.f;
    const float bondAmt = cachedParams.obblBond ? cachedParams.obblBond->load() : 0.f;
    const float dramaAmt = cachedParams.oleDrama ? cachedParams.oleDrama->load() : 0.f;

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
                dst.eng->applyCouplingInput(CouplingType::LFOToPitch, communeAmt, &bleedBuf, 1);
            }

            // OBBLIGATO BOND macro → AmpToFilter to siblings
            if (srcId == "Obbligato" && bondAmt > 0.001f)
            {
                const float bondBuf = srcSample * bondAmt;
                dst.eng->applyCouplingInput(CouplingType::AmpToFilter, bondAmt, &bondBuf, 1);
            }

            // OLE DRAMA macro → EnvToMorph to siblings
            if (srcId == "Ole" && dramaAmt > 0.001f)
            {
                const float dramaBuf = srcSample * dramaAmt;
                dst.eng->applyCouplingInput(CouplingType::EnvToMorph, dramaAmt, &dramaBuf, 1);
            }
        }
    }
}

void XOceanusProcessor::processBrothCoupling(std::array<SynthEngine*, MaxSlots>& enginePtrs)
{
    // Locate all four BROTH engines. Bail out early if any is absent — coupling
    // only activates when the full quad is loaded (same semantics as the Ghost Slot).
    OverwashEngine* overwash = nullptr;
    OverwornEngine* overworn = nullptr;
    OverflowEngine* overflow = nullptr;
    OvercastEngine* overcast = nullptr;

    for (auto* eng : enginePtrs)
    {
        if (!eng)
            continue;
        const juce::String id = eng->getEngineId();
        if (id == BrothCoordinator::kOverwash)
            overwash = static_cast<OverwashEngine*>(eng);
        else if (id == BrothCoordinator::kOverworn)
            overworn = static_cast<OverwornEngine*>(eng);
        else if (id == BrothCoordinator::kOverflow)
            overflow = static_cast<OverflowEngine*>(eng);
        else if (id == BrothCoordinator::kOvercast)
            overcast = static_cast<OvercastEngine*>(eng);
    }

    // All four must be present for BROTH coupling to activate.
    if (!overwash || !overworn || !overflow || !overcast)
        return;

    // ── Read OVERWORN's exported state ────────────────────────────────────────
    // OVERWORN is the "broth pot": it accumulates session-long reduction state
    // (spectral mass evaporating from high bands to low) and exports it so the
    // sibling engines can react to the age and concentration of the session.
    const float sessionAge = overworn->getSessionAge();
    const float concentrateDark = overworn->getConcentrateDark();
    const float totalSpectralMass = overworn->getTotalSpectralMass();

    // ── OVERWORN → OVERWASH ────────────────────────────────────────────────────
    // As the broth reduces (sessionAge rises), the diffusion medium grows more
    // viscous — OVERWASH's high-frequency diffusion slows accordingly.
    overwash->setBrothSessionAge(sessionAge);

    // ── OVERWORN → OVERFLOW ────────────────────────────────────────────────────
    // A concentrated, caramelised broth (high concentrateDark) is denser and
    // builds pressure faster — OVERFLOW's effective threshold drops.
    overflow->setBrothConcentrateDark(concentrateDark);

    // ── OVERWORN → OVERCAST ────────────────────────────────────────────────────
    // Depleted spectral mass means fewer available nucleation sites for ice
    // crystals — OVERCAST's crystal brightness scales with remaining spectral mass.
    overcast->setBrothSpectralMass(totalSpectralMass);
}

// ── Engine graveyard implementation ──────────────────────────────────────────

void XOceanusProcessor::depositInGraveyard(std::shared_ptr<SynthEngine> engine) noexcept
{
    // Called ONLY from the audio thread — no blocking, no allocation.
    int write = graveyardWritePos_.load(std::memory_order_relaxed);
    int read  = graveyardReadPos_.load(std::memory_order_acquire);
    int next  = (write + 1) % kGraveyardSize;

    if (next == read)
    {
        // Graveyard full — 16 engine swaps outran the message thread drain.
        // Release the shared_ptr without calling ~SynthEngine() so the audio
        // thread is not blocked.  The engine leaks.  This path should never
        // trigger in normal use (requires 16 simultaneous swaps before a
        // single message thread dispatch runs).
        engine.reset();
        return;
    }

    graveyard_[write] = std::move(engine);
    graveyardWritePos_.store(next, std::memory_order_release);
}

void XOceanusProcessor::drainGraveyard()
{
    // Called ONLY from the message thread (via MessageManager::callAsync).
    // ~SynthEngine() runs here — vector deallocations are safe off the RT thread.
    int read  = graveyardReadPos_.load(std::memory_order_relaxed);
    int write = graveyardWritePos_.load(std::memory_order_acquire);

    while (read != write)
    {
        graveyard_[read].reset(); // destructor runs here, on message thread
        read = (read + 1) % kGraveyardSize;
    }

    graveyardReadPos_.store(read, std::memory_order_release);
}

void XOceanusProcessor::loadEngine(int slot, const std::string& engineId)
{
    if (slot < 0 || slot >= MaxSlots)
        return;

    // #904: Open a new APVTS undo transaction so Cmd+Z can revert parameter
    // state to before the engine swap.  Engine structural state (slot routing,
    // coupling routes) is not APVTS-managed, so those revert via StructuralUndoManager
    // when it is promoted from Future/ — for now the APVTS transaction at minimum
    // gives users a undo boundary for macro / engine-specific parameter changes.
    // Guard: skip if called during setStateInformation() (no audio context yet)
    // since no interactive undo makes sense during preset restore.
    undoManager.beginNewTransaction("Load engine: " + juce::String(engineId));

    auto newEngine = EngineRegistry::instance().createEngine(engineId);
    if (!newEngine)
        return;

    newEngine->attachParameters(apvts);
    {
        const double sr = atomicSampleRate_.load(std::memory_order_relaxed);
        // #700: Only call prepare() if prepareToPlay() has already run (sr > 0).
        // If loadEngine() is called before the host calls prepareToPlay() — e.g.
        // during setStateInformation() at construction time — sr is 0.0 and passing
        // it would trigger sr=0 guards inside engine DSP modules (see #701).
        // prepareToPlay() will call prepare() on all engines with the correct rate
        // once the host has provided it.
        if (sr > 0.0)
        {
            newEngine->prepare(sr, currentBlockSize.load(std::memory_order_relaxed));
            newEngine->prepareSilenceGate(sr, currentBlockSize.load(std::memory_order_relaxed),
                                          silenceGateHoldMs(newEngine->getEngineId()));
        }
        // Give the new engine access to the shared host transport regardless
        // of whether prepare() has run yet — the engine caches the pointer and
        // reads from it in renderBlock(), which only runs after prepareToPlay().
        newEngine->setSharedTransport(&hostTransport);
        // Give the new engine a pointer to MPEManager so per-note expression
        // (pitch bend, pressure, slide) is live from the first rendered block.
        // issue #1237 — was never called; engines loaded at runtime saw nullptr.
        newEngine->setMPEManager(&mpeManager);

        // Wave 5 A1: Wire the global mod routing pointers into OrreryEngine.
        if (auto* orrery = dynamic_cast<OrreryEngine*>(newEngine.get()))
        {
            orrery->setGlobalCutoffModPtr(&globalCutoffModOffset_);
            orrery->setGlobalLFO1OutPtr(&globalLFO1_);
        }
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
        pending.outgoing = oldEngine;
        pending.fadeGain = 1.0f;
        pending.fadeSamplesRemaining =
            static_cast<int>(atomicSampleRate_.load(std::memory_order_relaxed) * CrossfadeMs * 0.001);
        // Release-store: makes the fields above visible to the audio thread
        // before it observes ready==true.
        pending.ready.store(true, std::memory_order_release);
    }

    // Atomic swap — audio thread sees the new engine on next block
    auto shared = std::shared_ptr<SynthEngine>(std::move(newEngine));
    std::atomic_store(&engines[slot], shared);

    // Increment generation counter so processBlock() can detect the swap without
    // comparing raw pointers to potentially-freed objects (#756).
    engineGeneration_.fetch_add(1, std::memory_order_release);

    // Update coupling matrix with the new engine pointer so it can resolve
    // IAudioBufferSink capabilities and activate/suspend AudioToBuffer routes.
    // Must pass the raw pointer so activeEngines[] is updated before sink resolution.
    // (W2 Audit CRITICAL-1: was reading stale activeEngines without this)
    couplingMatrix.notifyCouplingMatrixOfSwap(slot, engineId, shared.get());

    if (onEngineChanged)
        juce::MessageManager::callAsync(
            [this, slot]
            {
                if (onEngineChanged)
                    onEngineChanged(slot);
            });
}

void XOceanusProcessor::unloadEngine(int slot)
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
        pending.outgoing = oldEngine;
        pending.fadeGain = 1.0f;
        pending.fadeSamplesRemaining =
            static_cast<int>(atomicSampleRate_.load(std::memory_order_relaxed) * CrossfadeMs * 0.001);
        // Release-store: makes the fields above visible to the audio thread
        // before it observes ready==true.
        pending.ready.store(true, std::memory_order_release);
    }

    std::shared_ptr<SynthEngine> empty;
    std::atomic_store(&engines[slot], empty);

    // Increment generation counter so processBlock() can detect the swap without
    // comparing raw pointers to potentially-freed objects (#756).
    engineGeneration_.fetch_add(1, std::memory_order_release);

    // SRO: Clear profiler and auditor for this slot
    engineProfilers[slot].reset();
    sroAuditor.clearSlot(slot);

    if (onEngineChanged)
        juce::MessageManager::callAsync(
            [this, slot]
            {
                if (onEngineChanged)
                    onEngineChanged(slot);
            });
}

SynthEngine* XOceanusProcessor::getEngine(int slot) const
{
    if (slot >= 0 && slot < MaxSlots)
    {
        auto eng = std::atomic_load(&engines[slot]);
        return eng.get();
    }
    return nullptr;
}

// createEditor() is implemented in Source/UI/XOceanusEditor.cpp

// ── Wave 5 A1: Global mod route snapshot flush ──────────────────────────────
// Called on the message thread whenever the route table changes.
// Copies all active routes into a fixed-size array, then increments the
// atomic generation counter so the audio thread picks up the new snapshot on
// its next block.  No heap allocation, no mutex.
void XOceanusProcessor::flushModRoutesSnapshot() noexcept
{
    auto routes = modRoutingModel_.getRoutesCopy(); // message-thread allocation OK
    int count = 0;
    for (const auto& r : routes)
    {
        if (count >= kMaxGlobalRoutes)
            break;
        auto& snap = routesSnapshot_[static_cast<size_t>(count)];
        snap.sourceId  = r.sourceId;
        snap.depth     = r.depth;
        snap.bipolar   = r.bipolar;
        snap.valid     = true;
        snap.slotIndex = r.slotIndex; // C5: per-route slot index (-1 = N/A)
        // Copy dest param ID into fixed-length char array — no std::string on audio thread.
        // juce::String::copyToUTF8 writes at most maxBytes chars (inc. null terminator).
        r.destParamId.copyToUTF8(snap.destParamId, sizeof(snap.destParamId));

        // B1: Pre-resolve destination parameter pointer on the message thread so the
        // audio thread never calls apvts.getParameter() (a hash-map lookup, not RT-safe).
        // Lifetime of juce::RangedAudioParameter* == lifetime of apvts == lifetime of
        // this processor, so the pointer stays valid for the duration of the snapshot.
        snap.destParam = dynamic_cast<juce::RangedAudioParameter*>(
            apvts.getParameter(r.destParamId));

        // Pre-cache the range span (end - start) so the audio thread can scale
        // normalised modOffset to param units without calling any juce:: method.
        if (snap.destParam != nullptr)
        {
            const auto& range = snap.destParam->getNormalisableRange();
            snap.destParamRangeSpan = range.end - range.start;
        }
        else
        {
            snap.destParamRangeSpan = 0.0f;
        }

        // B1: Set isOrryCutoff flag by pointer identity — zero-cost strncmp replacement.
        // orryCutoffParam_ may be nullptr if Orrery is not loaded; guard accordingly.
        snap.isOrryCutoff = (orryCutoffParam_ != nullptr && snap.destParam == orryCutoffParam_);

        ++count;
    }
    // Zero out trailing slots so stale entries are not evaluated.
    for (int i = count; i < kMaxGlobalRoutes; ++i)
    {
        routesSnapshot_[static_cast<size_t>(i)].valid          = false;
        routesSnapshot_[static_cast<size_t>(i)].destParam       = nullptr;
        routesSnapshot_[static_cast<size_t>(i)].destParamRangeSpan = 0.0f;
        routesSnapshot_[static_cast<size_t>(i)].isOrryCutoff    = false;
    }

    routesSnapshotCount_.store(count, std::memory_order_relaxed);

    // Release fence: ensures all prior writes (routesSnapshot_[], routesSnapshotCount_,
    // cachedOrryFltCutoff_) are visible to the audio thread before it reads the
    // incremented version counter.  ARM-safe idiom (matching WaveformFifo pattern).
    std::atomic_thread_fence(std::memory_order_release);
    snapshotVersion_.fetch_add(1, std::memory_order_relaxed);
}

void XOceanusProcessor::getStateInformation(juce::MemoryBlock& destData)
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

    // Wave 5 A1 — Persist global mod routes as a ValueTree child ("modRoutes").
    // Existing child from a previous save is replaced first (guard against double-save).
    if (auto existing = state.getChildWithName("modRoutes"); existing.isValid())
        state.removeChild(existing, nullptr);
    state.appendChild(modRoutingModel_.toValueTree(), nullptr);

    // #1179 — Persist TideWaterline per-step sequence data as a ValueTree child.
    // D1 foundation: step patterns survive DAW session recall independently of
    // preset loads.  onGetTideWaterlineState is registered by OceanView after
    // waterline_ is initialised.  If the callback is null (editor not open), the
    // child is simply omitted — setStateInformation stores it for deferred pickup.
    if (onGetTideWaterlineState)
    {
        auto tideState = onGetTideWaterlineState();
        if (tideState.isValid())
        {
            if (auto existing = state.getChildWithName("TideWaterlineSteps"); existing.isValid())
                state.removeChild(existing, nullptr);
            state.appendChild(tideState, nullptr);
        }
    }

    // Wave 5 D1 — Persist XOuija grid contents (64 cells, all 3 layers).
    // Heatmap is intentionally NOT persisted — it is ephemeral session state
    // and resets on every load (spec section 6).
    // saveGridToValueTree() is message-thread-safe (called from getStateInformation).
    {
        if (auto existing = state.getChildWithName("XOuijaGrid"); existing.isValid())
            state.removeChild(existing, nullptr);
        state.appendChild(ouijaWalkEngine_.saveGridToValueTree(), nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml)
    {
        // FIX 4 / Fix #315 — State schema version (bump when format changes).
        // v1 = legacy (no slot/coupling/CM data)
        // v2 = slots + coupling + CM + mutes
        // v3 = + macroTargets + playScaleIndex + editorUIState (this build)
        xml->setAttribute("stateVersion", 3);

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
                re->setAttribute("src", r.sourceSlot);
                re->setAttribute("dst", r.destSlot);
                re->setAttribute("type", static_cast<int>(r.type));
                re->setAttribute("amount", static_cast<double>(r.amount));
                re->setAttribute("normalled", r.isNormalled);
                re->setAttribute("active", r.active);
            }
        }

        // FIX 3 — Save ChordMachine per-step sequencer data.  The APVTS
        // captures pattern-template selection but not custom per-step edits.
        juce::String cmStateJson = juce::JSON::toString(chordMachine.serializeState());
        xml->setAttribute("chordMachineState", cmStateJson);

        // FIX 7 — Save MacroSystem target wiring (closes #313).
        // Macro targets are in-memory only; without this they vanish on reload.
        // getState() serialises targets + labels to a child XmlElement so the
        // existing XML tree owns the lifetime — no raw-pointer leaks.
        if (auto macroElem = macroSystem_.getState())
            xml->addChildElement(macroElem.release());

        // FIX 8 — Save PlaySurface scale selector index (closes #314).
        xml->setAttribute("playScaleIndex", persistedPlayScaleIndex);

        // FIX 9 — Save editor UI state: selectedSlot, signalFlowActiveSection,
        // cockpitBypass (closes #357).
        xml->setAttribute("editorSelectedSlot", persistedSelectedSlot);
        xml->setAttribute("editorSignalFlowSection", persistedSignalFlowSection);
        xml->setAttribute("editorCockpitBypass", persistedCockpitBypass ? 1 : 0);

        // D4 — Save register lock + current register (per-instance, per-session).
        xml->setAttribute("registerLocked",  persistedRegisterLocked  ? 1 : 0);
        xml->setAttribute("registerCurrent", persistedRegisterCurrent);

        // §1.1.1 Principle 4 — Persist first-launch flag so Sound on First Launch
        // fires exactly once across all sessions.  Once saved, hasLaunchedBefore is
        // always true in subsequent saves and is never reset by the user.
        xml->setAttribute("hasLaunchedBefore", 1);

        copyXmlToBinary(*xml, destData);
    }
}

void XOceanusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        hasRestoredState = true; // Mark that saved state exists — skip default-engine load in prepareToPlay.

        // §1.1.1 Principle 4 — Restore first-launch flag.  Any saved state implicitly
        // means the plugin has launched at least once; the explicit attribute makes
        // the intent clear and handles forward-compatibility gracefully.
        // Default 0 handles old sessions saved before this feature (first run after
        // update would replay the experience — intentional, not a bug).
        if (xml->getIntAttribute("hasLaunchedBefore", 0) != 0)
            hasLaunchedBefore_.store(true, std::memory_order_release);

        apvts.replaceState(juce::ValueTree::fromXml(*xml));

        // FIX 4 / Fix #315 — Read schema version and apply forward migrations so
        // old sessions load safely into newer builds.
        //
        // Migration dispatch — each migrateVxToVy() mutates the XML in-place and
        // applies safe defaults for any new fields that did not exist in that version.
        // Migrations are additive-only: they never remove data, only add defaults.
        //
        // v1 → v2: slot selections, coupling routes, ChordMachine, slot mutes were
        //          not persisted in v1. Default: empty slots, no routes, default CM.
        // v2 → v3: macroTargets, playScaleIndex, editorUIState added. Defaults:
        //          no macro targets, scale 0 (Chromatic), no editor overrides.
        //
        // To add a new schema version in future:
        //   1. Bump stateVersion in getStateInformation() (above).
        //   2. Add a new migration block: if (loadedVersion < N+1) { /* apply defaults */ }
        //   3. Restore the new fields below inside the stateVersion >= N+1 guard.
        const int loadedVersion = xml->getIntAttribute("stateVersion", 1);

        // v1 → v2: no structural XML transform needed — the restore guards below
        // already gate on stateVersion >= 2 and fall back to defaults for absent attrs.

        // v2 → v3: macroTargets, playScaleIndex, editorUIState.
        // If missing (v2 save), the existing attribute-read fallbacks (0 / -1 / false)
        // are already the correct defaults, so no explicit XML patching is required.

        // Alias so existing >= 2 guards compile unchanged.
        const int stateVersion = loadedVersion;

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
                    r.sourceSlot = re->getIntAttribute("src", 0);
                    r.destSlot = re->getIntAttribute("dst", 1);
                    int typeInt = re->getIntAttribute("type", 0);
                    if (typeInt < 0 || typeInt > static_cast<int>(CouplingType::TriangularCoupling))
                        typeInt = 0;
                    r.type = static_cast<CouplingType>(typeInt);
                    r.amount = static_cast<float>(re->getDoubleAttribute("amount", 0.5));
                    r.isNormalled = re->getBoolAttribute("normalled", false);
                    r.active = re->getBoolAttribute("active", true);

                    // Bounds-check before adding to prevent corrupted saves
                    // from crashing or creating self-routing loops.
                    // addRoute() also enforces graph-level cycle detection for
                    // audio-rate types, so corrupted saves cannot install a
                    // feedback loop even if the self-route check is bypassed.
                    if (r.sourceSlot >= 0 && r.sourceSlot < MaxSlots && r.destSlot >= 0 && r.destSlot < MaxSlots &&
                        r.sourceSlot != r.destSlot)
                    {
                        couplingMatrix.addRoute(r);
                    }
                }
            }

            // FIX 3 — Restore ChordMachine per-step sequencer data.
            if (xml->hasAttribute("chordMachineState"))
            {
                juce::var cmState = juce::JSON::parse(xml->getStringAttribute("chordMachineState"));
                chordMachine.restoreState(cmState);
            }

            // FIX 7 — Restore MacroSystem target wiring (closes #313).
            // Must run after engines are loaded (FIX 1 above) so that
            // APVTS parameter pointers for engine params are valid when
            // setTargets() caches them inside setState().
            macroSystem_.setState(xml->getChildByName("MacroTargets"));
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
                onSetXOuijaState(uiStateTree);
        }

        // Wave 5 A1 — Restore global mod routes.
        // fromValueTree() is safe when the "modRoutes" child is absent (old sessions):
        // it returns without modifying the model.  After restoring, flush the snapshot
        // so processBlock evaluates the restored routes immediately.
        {
            auto modRoutesTree = apvts.state.getChildWithName("modRoutes");
            if (modRoutesTree.isValid())
            {
                modRoutingModel_.fromValueTree(modRoutesTree);
                flushModRoutesSnapshot();
            }
        }

        // #1179 — Restore TideWaterline per-step sequence data (D1 foundation).
        // "TideWaterlineSteps" child is absent in sessions predating this feature
        // — no-op in that case (algorithmic default pattern is kept on construction).
        // Case A: editor already open → callback registered, dispatch immediately.
        // Case B: editor not yet open → store for deferred pickup in initWaterline().
        {
            auto tideTree = apvts.state.getChildWithName("TideWaterlineSteps");
            if (tideTree.isValid())
            {
                if (onSetTideWaterlineState)
                    onSetTideWaterlineState(tideTree);
                else
                    persistedTideWaterlineState_ = tideTree;
            }
        }

        // Wave 5 D1 — Restore XOuija grid contents.
        // loadGridFromValueTree() enqueues 64 cell edits through the SPSC queue so
        // the audio thread receives them lock-free without a stall at the next block.
        // "XOuijaGrid" is absent in sessions predating this feature — no-op safe
        // (walk engine keeps its default-constructed neutral-cell grid).
        {
            auto gridTree = apvts.state.getChildWithName("XOuijaGrid");
            if (gridTree.isValid())
                ouijaWalkEngine_.loadGridFromValueTree(gridTree);
        }

        // FIX 8 — Restore PlaySurface scale selector index (closes #314).
        // Default 0 (Chromatic) matches PlayControlPanel init state — safe for
        // old sessions that predate this field.
        persistedPlayScaleIndex = xml->getIntAttribute("playScaleIndex", 0);

        // FIX 9 — Restore editor UI state (closes #357).
        // The editor reads these via getPersistedXxx() after construction.
        persistedSelectedSlot = xml->getIntAttribute("editorSelectedSlot", -1);
        persistedSignalFlowSection = xml->getIntAttribute("editorSignalFlowSection", 0);
        persistedCockpitBypass = xml->getIntAttribute("editorCockpitBypass", 0) != 0;

        // D4 — Restore register lock + current register.
        // Defaults: unlocked, Gallery (0) — graceful for sessions predating D4.
        persistedRegisterLocked  = xml->getIntAttribute("registerLocked",  0) != 0;
        persistedRegisterCurrent = xml->getIntAttribute("registerCurrent", 0);
        // Clamp to valid range (0=Gallery, 1=Performance, 2=Coupling).
        if (persistedRegisterCurrent < 0 || persistedRegisterCurrent > 2)
            persistedRegisterCurrent = 0;
    }
}

void XOceanusProcessor::applyPreset(const PresetData& preset)
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
                fullId = xoceanus::resolveSnapParamAlias(fullId);
                if (fullId.isEmpty())
                    continue; // param was removed — skip silently
            }

            // Resolve Overbite (Bite) legacy param aliases before lookup.
            // Four schema generations coexisted; this maps Gen 2/3 names to their
            // canonical Gen 4 APVTS IDs and drops params with no equivalent.
            if (fullId.startsWith("poss_"))
            {
                fullId = xoceanus::resolveBiteParamAlias(fullId);
                if (fullId.isEmpty())
                    continue; // param was removed — skip silently
            }

            // Try as-is first (already a full ID like "opal_source"),
            // then with prefix ("source" → "opal_source").
            if (apvts.getParameter(fullId) == nullptr)
                fullId = prefix + prop.name.toString();

            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(fullId)))
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
        auto syncParam = [&](const char* id, float val)
        {
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
                p->setValueNotifyingHost(p->convertTo0to1(val));
        };
        auto syncBool = [&](const char* id, bool val)
        {
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
    // This is the primary differentiator for XOceanus — coupling routes MUST
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
            if (s == "AmpToFilter")
            {
                out = CouplingType::AmpToFilter;
                return true;
            }
            if (s == "AmpToPitch")
            {
                out = CouplingType::AmpToPitch;
                return true;
            }
            if (s == "LFOToPitch")
            {
                out = CouplingType::LFOToPitch;
                return true;
            }
            if (s == "EnvToMorph")
            {
                out = CouplingType::EnvToMorph;
                return true;
            }
            if (s == "AudioToFM")
            {
                out = CouplingType::AudioToFM;
                return true;
            }
            if (s == "AudioToRing")
            {
                out = CouplingType::AudioToRing;
                return true;
            }
            if (s == "FilterToFilter")
            {
                out = CouplingType::FilterToFilter;
                return true;
            }
            if (s == "AmpToChoke")
            {
                out = CouplingType::AmpToChoke;
                return true;
            }
            if (s == "RhythmToBlend")
            {
                out = CouplingType::RhythmToBlend;
                return true;
            }
            if (s == "EnvToDecay")
            {
                out = CouplingType::EnvToDecay;
                return true;
            }
            if (s == "PitchToPitch")
            {
                out = CouplingType::PitchToPitch;
                return true;
            }
            if (s == "AudioToWavetable")
            {
                out = CouplingType::AudioToWavetable;
                return true;
            }
            if (s == "AudioToBuffer")
            {
                out = CouplingType::AudioToBuffer;
                return true;
            }
            if (s == "KnotTopology")
            {
                out = CouplingType::KnotTopology;
                return true;
            }
            if (s == "TriangularCoupling")
            {
                out = CouplingType::TriangularCoupling;
                return true;
            }
            // Legacy arrow-notation aliases
            if (s == "Amp->Filter")
            {
                out = CouplingType::AmpToFilter;
                return true;
            }
            if (s == "Amp->Pitch")
            {
                out = CouplingType::AmpToPitch;
                return true;
            }
            if (s == "LFO->Pitch")
            {
                out = CouplingType::LFOToPitch;
                return true;
            }
            if (s == "Env->Morph")
            {
                out = CouplingType::EnvToMorph;
                return true;
            }
            if (s == "Audio->FM")
            {
                out = CouplingType::AudioToFM;
                return true;
            }
            if (s == "Audio->Ring")
            {
                out = CouplingType::AudioToRing;
                return true;
            }
            if (s == "Filter->Filter")
            {
                out = CouplingType::FilterToFilter;
                return true;
            }
            if (s == "Amp->Choke")
            {
                out = CouplingType::AmpToChoke;
                return true;
            }
            if (s == "Rhythm->Blend")
            {
                out = CouplingType::RhythmToBlend;
                return true;
            }
            if (s == "Env->Decay")
            {
                out = CouplingType::EnvToDecay;
                return true;
            }
            if (s == "Pitch->Pitch")
            {
                out = CouplingType::PitchToPitch;
                return true;
            }
            if (s == "Audio->Wavetable")
            {
                out = CouplingType::AudioToWavetable;
                return true;
            }
            if (s == "Audio->Buffer")
            {
                out = CouplingType::AudioToBuffer;
                return true;
            }
            if (s == "Knot->Topology")
            {
                out = CouplingType::KnotTopology;
                return true;
            }
            if (s == "Triangular->Coupling")
            {
                out = CouplingType::TriangularCoupling;
                return true;
            }
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
                DBG("applyPreset: coupling route skipped — engineA='" + canonA + "' (slot=" + juce::String(slotA) +
                    ") engineB='" + canonB + "' (slot=" + juce::String(slotB) + ") not found in active slots");
                continue;
            }

            if (slotA == slotB)
                continue;

            CouplingType couplingType{};
            if (!stringToCouplingType(cp.type, couplingType))
            {
                DBG("applyPreset: unknown coupling type '" + cp.type + "' — route skipped");
                continue;
            }

            // Preserve bipolar amounts for types that use them (e.g. AmpToPitch,
            // PitchToPitch). Only force unipolar for inherently one-directional types.
            // Audit finding: P0-1 CRITICAL-1 — std::abs() was destroying negative amounts.
            const bool unipolarOnly =
                (couplingType == CouplingType::AmpToChoke || couplingType == CouplingType::AudioToBuffer ||
                 couplingType == CouplingType::AudioToWavetable);
            const float clampedAmount =
                unipolarOnly ? juce::jlimit(0.0f, 1.0f, std::abs(cp.amount)) : juce::jlimit(-1.0f, 1.0f, cp.amount);

            MegaCouplingMatrix::CouplingRoute route;
            route.sourceSlot = slotA;
            route.destSlot = slotB;
            route.type = couplingType;
            route.amount = clampedAmount;
            route.isNormalled = false;
            route.active = true;

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

                if (scratchMatrix.wouldCreateCycle(route.sourceSlot, route.destSlot, route.type))
                {
                    DBG("applyPreset: audio-rate cycle detected — skipping route " + canonA + "(slot " +
                        juce::String(slotA) + ") → " + canonB + "(slot " + juce::String(slotB) + ") [" + cp.type + "]");
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
            DBG("applyPreset: " + juce::String((int)preset.couplingPairs.size()) +
                " coupling routes could not be restored — required engines not loaded. " +
                "Existing coupling routes preserved.");
        }
    }

    // #678 — Wire preset macro targets (CHARACTER/MOVEMENT/COUPLING/SPACE) into
    // MacroSystem so that sweeping each macro knob modulates the assigned engine
    // parameters. Previously macroTargets were parsed from the preset JSON but
    // never applied to macroSystem_, so M3 (COUPLING) and other macros had no
    // effect on engine parameters during performance.
    //
    // MacroSystem::clearAllTargets() + setTargets() is safe to call here
    // (message thread, outside the audio callback). The SpinLock inside
    // MacroSystem guarantees the audio thread sees a consistent snapshot.
    {
        macroSystem_.clearAllTargets();

        // Apply macro labels (CHARACTER / MOVEMENT / COUPLING / SPACE or preset overrides).
        for (int macroIdx = 0; macroIdx < 4; ++macroIdx)
        {
            if (macroIdx < preset.macroLabels.size() && preset.macroLabels[macroIdx].isNotEmpty())
                macroSystem_.setLabel(macroIdx, preset.macroLabels[macroIdx]);
        }

        // Convert PresetMacroTarget → MacroTarget and assign to each slot.
        for (int macroIdx = 0; macroIdx < 4; ++macroIdx)
        {
            const auto& slotTargets = preset.macroTargets[static_cast<size_t>(macroIdx)];
            if (slotTargets.empty())
                continue;

            std::vector<MacroTarget> liveTargets;
            liveTargets.reserve(slotTargets.size());
            for (const auto& pmt : slotTargets)
            {
                MacroTarget mt;
                mt.engineId    = pmt.engineName;
                mt.parameterId = pmt.paramId;
                mt.minValue    = pmt.depthMin;
                mt.maxValue    = pmt.depthMax;
                // isCouplingTarget / couplingRouteSlot remain false/-1:
                // engine parameter targets are resolved via parameterId lookup.
                liveTargets.push_back(std::move(mt));
            }
            macroSystem_.setTargets(macroIdx, std::move(liveTargets));
        }

        // M3 (COUPLING macro, index 2) also scales all active coupling route
        // amounts proportionally so sweeping it from 0→1 morphs coupling depth.
        // Wire M3 to all four coupling route slots (cp_r1_amount … cp_r4_amount).
        // Routes that aren't active have their amount parameter at 0 already,
        // so this has no effect on uncoupled presets.
        for (int routeSlot = 0; routeSlot < 4; ++routeSlot)
            macroSystem_.addCouplingTarget(MacroSystem::CouplingMacroIndex, routeSlot, 0.0f, 1.0f);
    }

    // Phase 0 wildcard infrastructure: publish preset DNA into the DNAModulationBus.
    // Preset DNA is global (one struct per preset); we apply it to all 4 engine slots
    // as the same baseline. Future enhancement: per-engine DNA in preset schema would
    // call setBaseDNA per slot with different values.
    {
        std::array<float, 6> dnaArr = {
            preset.dna.brightness, preset.dna.warmth,    preset.dna.movement,
            preset.dna.density,    preset.dna.space,     preset.dna.aggression
        };
        for (int slot = 0; slot < xoceanus::DNAModulationBus::MaxEngineSlots; ++slot)
            dnaBus_.setBaseDNA(slot, dnaArr);
    }
}

} // namespace xoceanus

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new xoceanus::XOceanusProcessor();
}
