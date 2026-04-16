# Changelog

All notable changes to XOceanus are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Version numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Entries are added to `[Unreleased]` during development; the section is
re-titled when a version is cut.

---

## [Unreleased]

### Added
- OBIONT engine (`obnt_` prefix) — Cellular automata oscillator; 1D Wolfram CA spatial projection + cosine readout + 8-voice poly + anti-extinction
- OKEANOS engine (`okan_` prefix) — Cardamom warmth synth
- OUTFLOW engine (`out_` prefix) — Deep storm indigo current synth
- OVERLAP engine (`olap_` prefix) — Stereo bioluminescence; KnotMatrix + FDN + Voice + Entrainment DSP; real DSP safety fixes applied in latest session
- CMake install() rules and CPack config for distribution packaging (#407)
- SPDX-License-Identifier and copyright headers to all `Source/` .h files (#397)
- SpinLock thread safety to ShaperRegistry slot arrays (#557)
- Screen reader accessibility metadata to ChordMachinePanel and ObrixDetailPanel (#392)
- Descriptive tooltips to ObrixDetailPanel knobs and CouplingInspectorPanel route cards (#389)
- Proportional reflow in OutshineMainComponent `resized()` (#227)
- JUCE_ASSERT_MESSAGE_THREAD annotations to CouplingVisualizer (#198)
- Per-destination LFO routing in MegaCouplingMatrix (#178)
- Zone-map click now scrolls and highlights grain in strip (#169)
- GalleryColors unified reduce-motion API in XOuijaPanel (#223)
- Navigation links for 19 previously orphaned site pages (#323)
- Alignment Rite page reference restored in documentation (#405)
- HTTPS enforcement before API key transmission in SoundAssistant (#431)
- `engines.json` roster file — machine-readable engine manifest for CI/build automation
- `.github/ISSUE_TEMPLATE/engine_fix.md` — structured template for doctrine violations and seance score regressions

### Changed
- Accent colors for Oven and Ogre updated to accessible distinct colors — no longer near-black (#185)
- ObrixDetailPanel and ParameterGrid labels updated to prevent text clipping (#391)
- All hardcoded pixel sizes replaced with proportional/font-metric sizing (#386)
- Ghost slot animation in CouplingVisualizer now guarded by `EngineRegistry::isRegistered()` check (#188)
- `chaosAmount` signature corrected in relevant coupling routes (#163)

### Fixed
- 0 cross-mood duplicate preset names confirmed; dedup strategy documented (#236)
- Security: HTTPS enforced before API key transmission in SoundAssistant (#431)
- OceanDeepEngine: latest DSP stability pass applied

---

## [0.9.0] — 2026-03-29 — FATHOM QDD Level 5 Fleet Certification

### Added
- FATHOM QDD Level 5 fleet audit complete: full fleet + DSP lib + core, 20 agents, 47 CRITs resolved
- Fleet average score: 8.1/10 post-audit
- Ship-ready engines (≥9.0): ONSET, OWARE, OPALINE, OPTIC, ORGANON, OXYTOCIN, OSTERIA, OUROBOROS, OSTINATO, OXBOW, OBSCURA
- TIDEsigns UI/UX audit: 450 findings, score 5.4→7.2; 4 LOCKED design decisions documented
- Outshine C++ DSP engine (1,018 lines); XOriginate.h OriginateConfig API
- Export Pyramid architecture: ORIGINATE → OUTSHINE → OXPORT
- `/phantom-sniff` skill (6 hallucination-detection sniffer dogs, PostToolUse hook)
- Coupling UI architecture documentation (`Docs/coupling-ui-architecture-2026-03-21.md`)

### Changed
- XPN toolchain: QDD 3.6→7.5/10; all 11 BLOCKs resolved
- `xoutshine.py` archived — Python duplicate superseded by Originate UI + Oxport CLI

---

## [0.8.0] — 2026-03-26 — QDD Sprint + Platform Architecture

### Added
- OXYTOCIN engine (`oxy_` prefix) — Circuit × Love topology; Note Duration (B040); 9.5/10 fleet leader
- OUTLOOK engine (`look_` prefix) — Panoramic visionary synth; dual wavetable + parallax stereo
- OSMOSIS engine (`osmo_` prefix) — External audio membrane; envelope follower + pitch detect
- OPERA engine (`opera_` prefix) — Additive-vocal Kuramoto synchronicity; OperaConductor (B035); 8.85/10
- OFFERING engine (`ofr_` prefix) — Psychology-as-DSP (B038, B039); boom bap drums; 8.8/10
- OWARE engine (`owr_` prefix) — Tuned Akan percussion; Chaigne 1997 mallet physics (B032–B034); 9.2/10
- OXBOW engine (`oxb_` prefix) — Chiasmus FDN entangled reverb synth; 9.0/10
- OBRIX Wave 4 — Biophonic Synthesis: Harmonic Field, Environmental, Brick Ecology, Stateful Synthesis, FX Mode (14 new params)
- OBRIX Wave 5 — Reef Residency: 4-mode coupling ecology (Off/Competitor/Symbiote/Parasite)
- Kitchen Collection — All 6 quads complete with Guru Bin retreats: Chef (Organs), Kitchen (Pianos), Cellar (Bass), Garden (Strings), Broth (Pads), Fusion (EP)
- 161 awakening presets across 16 Kitchen Collection engines
- B040 blessed: Note Duration as Synthesis Parameter (OXYTOCIN)
- B041 blessed: Dark Cockpit Attentional Design — 5-level opacity hierarchy
- B042 blessed: Planchette as Autonomous Entity
- B043 blessed: Gesture Trail as First-Class Modulation Source
- QDD Sprint waves 1–6 complete: 3 items deferred (ORGANON/OPCODE color dup, OTO/OVERBITE near-white, currentSampleRate race)
- iOS OBRIX Pocket design — brick-based collectible synth; SpriteKit + JUCE hybrid; build milestone A+B complete
- Platform architecture: iPhone=Reef (OBRIX Pocket), iPad=School (OBRIX Academy), Desktop=Ocean (XOceanus)
- OBRIX Pocket gameplay depth locked: 30 decisions including Dive, Imprinting, Coupling Chemistry, NFC trading

### Changed
- Product rename: XOmnibus → XOlokun → **XOceanus** (canonical from 2026-03-24)
- Full XOceanus iOS port cancelled; replaced by OBRIX Pocket iPhone + OBRIX Academy iPad (2026-03-26)
- AU PASS: 0 errors, 492 warnings (all accepted)

---

## [0.7.0] — 2026-03-21 — Fleet Expansion Wave + Doctrine Completion

### Added
- OBRIX engine (`obrix_` prefix) — Flagship modular brick synthesis; 81 params across 5 waves
- ORBWEAVE engine (`weave_` prefix) — Topological knot coupling; Kelp Knot architecture (B021, B022)
- OVERTONE engine (`over_` prefix) — Continued fraction spectral engine; Nautilus (B028)
- ORGANISM engine (`org_` prefix) — Cellular automata generative engine; Coral Colony
- OUIE engine (`ouie_` prefix) — Duophonic hammerhead synth; HAMMER axis (B025–B027)
- OSTINATO engine (`osti_` prefix) — Modal membrane synthesis; 96 rhythm patterns (B017–B020)
- OPENSKY engine (`sky_` prefix) — Euphoric shimmer synth; Shepard Shimmer Architecture (B023, B024)
- OCEANDEEP engine (`deep_` prefix) — Deep-pressure submerged synthesis (B029–B031)
- OSPREY engine (`osprey_` prefix) — ShoreSystem turbulence-modulated resonator (B012)
- OSTERIA engine (`osteria_` prefix) — Ensemble synthesis; ShoreSystem shared (B012)
- OPALINE, OGRE, OLATE, OAKEN, OMEGA, ORCHARD, OVERGROW, OSIER, OXALIS, OVERWASH, OVERWORN, OVERFLOW, OVERCAST, OASIS, ODDFELLOW, ONKOLO, OPCODE engines (Kitchen Collection family)
- `Source/DSP/Effects/AquaticFXSuite.h` — 6 water-themed master effects
- `Source/DSP/Effects/MathFXChain.h` — Entropy, Voronoi, Quantum, Attractor processors
- `Source/DSP/Effects/BoutiqueFXChain.h` — Anomaly, Archive, Cathedral, Submersion generators
- fXOnslaught, fXObscura, fXOratory singularity FX engines
- `SDK/include/xoceanus/` — JUCE-free SDK headers for third-party engine development
- D006 mod wheel coverage: 22/22 original Prism Sweep engines (Round 12C)

### Changed
- D001 filter envelopes: RESOLVED fleet-wide — all engines have velocity-scaled filter envelopes (Round 9E)
- D004 dead params: RESOLVED fleet-wide — all declared parameters wired to DSP (Round 3B)
- D005 LFO breathing: RESOLVED fleet-wide — all engines have rate floor ≤ 0.01 Hz

### Fixed
- OBESE LFO1 exposed; Mojo (B015) now breathes — seance 6.6→~8.5
- ODDOSCAR LFO1 added; aftertouch→resonance wired — seance 6.9→~8.5
- ODDFELIX LFO rates exposed; aftertouch-rate wired — seance ~7.0→~8.5
- OCELOT biome crossfade was dead (`setBiomeTarget()` never called) — now live — seance 6.4→~8.5
- ORPHICA buffer extended to 1s; velocity→body resonance frequency wired — seance 8.0→~8.7
- OPERA SVF block-rate coefficient cache committed — eliminates per-sample recomputation click
- Tom double-saturation in OFFERING fixed post-seance

---

## [0.6.0] — 2026-03-14 — Prism Sweep Complete + Constellation Engines

### Added
- OVERLAP engine (`olap_` prefix) — Stereo bioluminescence; PolyBLEP; KnotMatrix; FDN
- OUTWIT engine (`owit_` prefix) — Chromatophore timbre engine; cellular automata step-synth
- OMBRE engine (`ombre_` prefix) — Dual-narrative engine; memory/forgetting + perception
- ORCA engine (`orca_` prefix) — Apex predator engine; wavetable + echolocation + breach
- OCTOPUS engine (`octo_` prefix) — Decentralized alien intelligence; arms + chromatophores + ink cloud
- OWLFISH engine (`owl_` prefix) — Monophonic subharmonic organism; Mixtur-Trautonium oscillator (B014)
- OBLIQUE engine (`oblq_` prefix) — Prismatic bounce synth; RTJ × Funk × Tame Impala
- OPTIC engine (`optic_` prefix) — Visual modulation engine; AutoPulse trance mode; Winamp-style visualizer (B005)
- OCELOT engine (`ocelot_` prefix) — Canopy-layered ecosystem synth; biome system
- OBBLIGATO, OHM, OLE, ORPHICA, OTTONI — Family engine stubs (Constellation group)
- `FamilyWaveguide.h` — Shared physical modeling DSP for XOrphica Pentagon architecture
- 5 Constellation engines added to fleet (B021–B024 blessings ratified)

### Changed
- Prism Sweep 12-round quality pass complete across all 24 original engines
- All 6 Doctrines resolved fleet-wide (D001–D006) for Prism Sweep cohort
- ~15,500 presets (up from 10,028 at sweep start); 0 duplicates; 100% 6D DNA coverage
- Fleet health score: ~92/100

### Fixed
- All 22 MIDI-capable engines: mod wheel wired (22/22)
- All 23 engines: aftertouch wired (Optic intentionally exempt as visual engine)
- Build PASS + `auval -v aumu Xolk XoOx` PASS

---

## [0.5.0] — 2026-03-11 — Original Fleet Seanced

### Added
- 47 original engines seanced by ghost council (Buchla, Schulze, Kakehashi, Tomita, Pearlman, Vangelis, Moog, Smith)
- Blessings B001–B013 ratified (Group Envelope System through Chromatophore Modulator)
- `MegaCouplingMatrix.h` — Cross-engine modulation; 15 coupling types including KnotTopology + TriangularCoupling
- PlaySurface — 4-zone unified playing interface (Pad/Fretless/Drum modes)
- `.xometa` preset format — 15 moods, 4 macros, 6D Sonic DNA
- Gallery Model UI — warm white shell framing engine accent colors; dark mode default
- XPN export pipeline for MPC compatibility

### Changed
- All engine parameter IDs namespaced to `{shortname}_{paramName}` convention and frozen
- `resolveEngineAlias()` added to PresetManager.h for legacy name migration

---

## [0.1.0] — 2026-03-01 — Initial Fleet Build

### Added
- 24 original Prism Sweep engines registered and built: OddfeliX, OddOscar, Overdub, Odyssey, Oblong, Obese, Onset, Overworld, Opal, Orbital, Organon, Ouroboros, Obsidian, Origami, Oracle, Obscura, Oceanic, Ocelot, Overbite, Optic, Oblique, Osprey, Osteria, Ouroboros (initial)
- `SynthEngine` interface — `Source/Core/SynthEngine.h`
- `EngineRegistry` — factory + 4-slot management
- `PresetManager.h` — .xometa loading/saving; `frozenPrefixForEngine` map
- CMakeLists.txt — universal binary (arm64/x86_64); JUCE 8 via FetchContent
- AU plugin code: `aumu Xolk XoOx`
- ~10,028 factory presets at initial Prism Sweep start

[Unreleased]: https://github.com/BertCalm/XO_OX-XOmnibus/compare/v0.9.0...HEAD
[0.9.0]: https://github.com/BertCalm/XO_OX-XOmnibus/compare/v0.8.0...v0.9.0
[0.8.0]: https://github.com/BertCalm/XO_OX-XOmnibus/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/BertCalm/XO_OX-XOmnibus/compare/v0.6.0...v0.7.0
[0.6.0]: https://github.com/BertCalm/XO_OX-XOmnibus/compare/v0.5.0...v0.6.0
[0.5.0]: https://github.com/BertCalm/XO_OX-XOmnibus/compare/v0.1.0...v0.5.0
[0.1.0]: https://github.com/BertCalm/XO_OX-XOmnibus/releases/tag/v0.1.0
