# XOceanus Engine Fleet — Complete Inventory (March 28, 2026)

## Executive Summary

**73 total engines registered** across the XO_OX-XOceanus synth platform:
- **4 adapter-based** (Opera, Oxytocin, Outwit, Overlap) — standalone repos integrated via adapters
- **69 native engines** — implemented directly in Source/Engines/
- **7 legacy aliases** (Bite/Bob/Drift/Dub/Fat/Morph/Snap) — map to canonical O-prefix names
- **17,251 total factory presets** (13,260 non-quarantine + 3,991 quarantine)
- **Fleet health:** Median 8.5/10, 3 engines at 9.0+, 6 total at 9.0+ (including estimates)

---

## Engine Registry (EngineRegistry.h)

**Location:** `Source/Core/EngineRegistry.h` (154 lines)

Key features:
- Factory pattern: `EngineFactory` function type for creating engine instances
- Static registration at compile time via `registerEngine()` calls
- 5-slot management (4 primary + 1 "Ghost Slot" for Kitchen Collection quads)
- 6 Kitchen Collection quads hardcoded:
  - Chef (Organs): OTO, OCTAVE, OLEG, OTIS
  - Kitchen (Pianos): OVEN, OCHRE, OBELISK, OPALINE
  - Cellar (Bass): OGRE, OLATE, OAKEN, OMEGA
  - Garden (Strings): ORCHARD, OVERGROW, OSIER, OXALIS
  - Broth (Pads): OVERWASH, OVERWORN, OVERFLOW, OVERCAST
  - Fusion (EP): OASIS, ODDFELLOW, ONKOLO, OPCODE

---

## All 76 Engines — Complete Listing with File Sizes

### Legacy Alias Engines (7)

| # | Dir | Canonical ID | Primary File | Size | Presets | Seance Score |
|----|-----|-------------|--------------|------|--------:|-------------|
| 1 | Bite | Overbite | BiteEngine.h | 102K | 255 | Full approval |
| 2 | Bob | Oblong | BobEngine.h | 65K | 802 | ~8.5 est. |
| 3 | Drift | Odyssey | DriftEngine.h | 69K | 640 | 7.6 |
| 4 | Dub | Overdub | DubEngine.h | 50K | 552 | 7.4 |
| 5 | Fat | Obese | FatEngine.h | 68K | 408 | ~8.5 est. |
| 6 | Morph | OddOscar | MorphEngine.h | 63K | 444 | ~8.5 est. |
| 7 | Snap | OddfeliX | SnapEngine.h | 55K | 531 | ~8.5 est. |

**Total Legacy:** 7 dirs, 502K combined source, 3,632 presets

---

### Native XOceanus Engines (66) — Alphabetical by Directory

| # | Dir | Canonical ID | Primary File | Size | Presets | Seance | Status |
|----|-----|-------------|--------------|------|--------:|--------|--------|
| 8 | Oaken | Oaken | OakenEngine.h | 35K | 30 | 8.4 | Kitchen/Cellar |
| 9 | Oasis | Oasis | OasisEngine.h | 37K | 20 | 8.7 | Kitchen/Fusion |
| 10 | Obbligato | Obbligato | ObbligatoEngine.h | 27K | 229 | 7.8 | No retreat |
| 11 | Obelisk | Obelisk | ObeliskEngine.h | 49K | 28 | 8.8 | Kitchen/Piano; P0 3 dead params |
| 12 | Oblique | Oblique | ObliqueEngine.h | 87K | 289 | ~7.2 | No retreat |
| 13 | Obrix | Obrix | ObrixEngine.h | 105K | 466 | 9.4 roadmap | **FLAGSHIP** — Wave 5 complete |
| 14 | Obscura | Obscura | ObscuraEngine.h | 81K | 143 | HIGH/unanimous | Physics-as-synthesis |
| 15 | Obsidian | Obsidian | ObsidianEngine.h | 71K | 280 | ~8.2 | No retreat |
| 16 | OceanDeep | OceanDeep | OceandeepEngine.h | 42K | 310 | 7.8 | No retreat |
| 17 | Oceanic | Oceanic | OceanicEngine.h | 56K | 185 | 7.1 | Velocity resolved |
| 18 | Ocelot | Ocelot | OcelotEngine.h (10K + 13 DSP modules) | 136K total | 271 | ~8.5 | EcosystemMatrix live |
| 19 | Ochre | Ochre | OchreEngine.h | 43K | 33 | 8.2 | Kitchen/Piano; P0 LFO2 dead |
| 20 | Octave | Octave (Chef) | OctaveEngine.h | 47K | 25 | 8.01→~8.7 | Chef quad; thin presets |
| 21 | Octopus | Octopus | OctopusEngine.h | 63K | 257 | 8.3→~8.5 | Aftertouch fixed |
| 22 | Oddfellow | Oddfellow | OddfellowEngine.h | 31K | 21 | 8.5 | Kitchen/Fusion |
| 23 | Offering | Offering | OfferingEngine.h (45K + 6 city modules) | 113K total | 154 | **8.8** | Psychology-as-DSP; B038/B039 blessed |
| 24 | Ogre | Ogre | OgreEngine.h | 29K | 29 | 7.9 | P0 soil bug; Kitchen/Cellar |
| 25 | Ohm | Ohm | OhmEngine.h | 30K | 236 | 7.6 | No retreat |
| 26 | Olate | Olate | OlateEngine.h | 31K | 29 | 8.1 | Kitchen/Cellar; thin presets |
| 27 | Ole | Ole | OleEngine.h | 19K | 216 | 7.0 | isHusband regression |
| 28 | Oleg | Oleg | OlegEngine.h | 53K | 25 | 8.0 | Chef quad; thin presets |
| 29 | Ombre | Ombre | OmbreEngine.h | 42K | 226 | ~8.0 | No retreat |
| 30 | Omega | Omega | OmegaEngine.h | 29K | 27 | 8.6 | Kitchen/Cellar; thin presets |
| 31 | Onkolo | Onkolo | OnkoloEngine.h | 29K | 20 | 8.8 | Kitchen/Fusion; thin presets |
| 32 | Onset | Onset | OnsetEngine.h | 96K | 480 | Ahead of industry | XVC; B002 blessed |
| 33 | Opal | Opal | OpalEngine.h (96K + 26K DSP) | 122K | 357 | APPROVED | Granular coupling crown jewel |
| 34 | Opaline | Opaline | OpalineEngine.h | 45K | 33 | 8.9 | Kitchen/Piano; P0 coupling dead |
| 35 | Opcode | Opcode | OpcodeEngine.h | 32K | 20 | **9.0** | Kitchen/Fusion; highest Kitchen score |
| 36 | OpenSky | OpenSky | OpenSkyEngine.h | 59K | 385 | 8.1 | D004 partial (sky_subWave) |
| 37 | Opera | Opera | OperaEngine.h (68K + 8 modules) | 152K | 156 | **8.85** | Adapter-based; Kuramoto conductor; B035–B037 |
| 38 | Optic | Optic | OpticEngine.h | 52K | 308 | Revolutionary | Zero-audio paradigm; B005 |
| 39 | Oracle | Oracle | OracleEngine.h | 75K | 241 | 8.6 | GENDY + Maqam; B010 |
| 40 | Orbital | Orbital | OrbitalEngine.h | 80K | 262 | APPROVED | Group Envelope System; B001 |
| 41 | Orbweave | Orbweave | OrbweaveEngine.h | 48K | 451 | 8.4 | Kelp Knot; B021–B022 |
| 42 | Orca | Orca | OrcaEngine.h | 57K | 270 | ~8.6 | Echolocation; aftertouch fixed |
| 43 | Orchard | Orchard | OrchardEngine.h | 34K | 30 | 8.2 | Garden quad; thin presets |
| 44 | Organism | Organism | OrganismEngine.h | 42K | 433 | 8.1/7.2 | Cellular automata; CA patched 2026-03-20 |
| 45 | Organon | Organon | OrganonEngine.h | 80K | 365 | **8/8 PASS** | VFE metabolism; B011; academic-grade |
| 46 | Origami | Origami | OrigamiEngine.h | 87K | 216 | APPROVED | STFT fold synthesis |
| 47 | Orphica | Orphica | OrphicaEngine.h | 31K | 264 | ~8.7 | Buffer extended; velocity→resonance |
| 48 | Osier | Osier | OsierEngine.h | 33K | 28 | 8.3 | Garden quad; thin presets |
| 49 | Osmosis | Osmosis | OsmosisEngine.h | 16K | 1 | **NOT SEANCED** | Design phase; external audio membrane |
| 50 | Osprey | Osprey | OspreyEngine.h | 95K | 166 | APPROVED | ShoreSystem B012 shared |
| 51 | Osteria | Osteria | OsteriaEngine.h | 90K | 234 | Production-grade | ShoreSystem B012 shared |
| 52 | Ostinato | Ostinato | OstinatoEngine.h | 97K | 229 | **8.7** | Modal membrane; B017–B020 |
| 53 | Otis | Otis | OtisEngine.h | 65K | 27 | 7.8 | Leslie Doppler wrong; Chef quad |
| 54 | Oto | Oto | OtoEngine.h | 45K | 34 | **8.6 post-fix** | Hammond organ; Chef quad |
| 55 | Ottoni | Ottoni | OttoniEngine.h | 23K | 222 | 7.2 | Dead params fixed |
| 56 | Ouie | Ouie | OuieEngine.h | 77K | 396 | **8.5** | Duophonic; B025–B027; no retreat |
| 57 | Ouroboros | Ouroboros | OuroborosEngine.h | 84K | 346 | Production-ready | Chaos attractors; B003/B007 |
| 58 | Outlook | Outlook | OutlookEngine.h | 34K | 18 | **NOT SEANCED** | New engine (2026-03-23/24); Panoramic visionary |
| 59 | Outwit | Outwit | XOutwitAdapter.h | 41K | 593 | **8.7** | Adapter-based; Cellular automata stepsynth |
| 60 | Oven | Oven | OvenEngine.h | 48K | 28 | 8.7 | Piano; Kitchen/Piano; thin presets |
| 61 | Overcast | Overcast | OvercastEngine.h | 34K | 30 | 7.9 | Pad; Kitchen/Broth; BROTH coordinator not written |
| 62 | Overflow | Overflow | OverflowEngine.h | 34K | 30 | 8.0 | Pad; Kitchen/Broth; BROTH coordinator not written |
| 63 | Overgrow | Overgrow | OvergrowEngine.h | 35K | 27 | 7.5 | Bow noise injection bug; Garden quad |
| 64 | Overlap | Overlap | XOverlapAdapter.h | 41K | 577 | **8.4** | Adapter-based; KnotMatrix FDN |
| 65 | Overtone | Overtone | OvertoneEngine.h | 43K | 373 | 7.6 | Continued fractions; Nautilus; pi patched |
| 66 | Overwash | Overwash | OverwashEngine.h | 34K | 30 | 7.8 | Pad; Kitchen/Broth; BROTH coordinator not written |
| 67 | Overworld | Overworld | OverworldEngine.h (32K + 18K params) | 50K | 299 | 7.6 | ERA triangle; B009 |
| 68 | Overworn | Overworn | OverwornEngine.h | 37K | 30 | **8.6** | Pad; Kitchen/Broth; BROTH coordinator not written |
| 69 | Oware | Oware | OwareEngine.h | 46K | 175 | **8.7** (→9.2 projected) | Tuned percussion; Mallet physics; B032–B034 |
| 70 | Owlfish | Owlfish | OwlfishEngine.h (11K + 12 DSP modules) | 86K | 157 | 7.1 | Mixtur-Trautonium; B014 |
| 71 | Oxalis | Oxalis | OxalisEngine.h | 31K | 27 | 7.8 | Garden quad; thin presets; MW not routed |
| 72 | Oxbow | Oxbow | OxbowEngine.h | 32K | 175 | **9.0** | Entangled reverb synth; FDN + phase erosion |
| 73 | Oxytocin | Oxytocin | OxytocinEngine.h (10K + 10 modules) | 54K | 130 | **9.5 est.** | **FLEET LEADER**; Circuit × Love; B040; adapter-based |

**Total Native:** 66 engines, ~3,000K+ combined source, 13,260+ presets (non-quarantine)

---

## Kitchen Collection (24 Engines in 6 Quads)

All 6 quads built, seanced, and guru bin retreats complete (2026-03-23):

| Quad | Theme | Engines | Presets | Avg Score | Status |
|------|-------|---------|--------:|-----------|--------|
| **Chef** (Organs) | Adversarial coupling | OTO, OCTAVE, OLEG, OTIS | 111 | 8.1 | Retreat complete; Leslie Doppler fix needed (Otis) |
| **Kitchen** (Pianos) | Modal resonator banks | OVEN, OCHRE, OBELISK, OPALINE | 122 | 8.6 | Retreat complete; Obelisk/Ochre/Opaline P0 fixes needed |
| **Cellar** (Bass) | Gravitational coupling | OGRE, OLATE, OAKEN, OMEGA | 115 | 8.3 | Retreat complete; Ogre soil bug fix needed |
| **Garden** (Strings) | Growth Mode | ORCHARD, OVERGROW, OSIER, OXALIS | 112 | 8.1 | Retreat complete; Overgrow bow noise + Oxalis MW fix |
| **Broth** (Pads) | Multi-timescale diffusion | OVERWASH, OVERWORN, OVERFLOW, OVERCAST | 120 | 8.1 | Retreat complete; **BROTH COORDINATOR NOT WRITTEN** (critical) |
| **Fusion** (EP) | Spectral Fingerprint Cache | OASIS, ODDFELLOW, ONKOLO, OPCODE | 94 | 8.7 | Retreat complete; Opcode highest (9.0) |

**Total Kitchen Collection:** 24 engines, 674 presets, avg 8.3/10

---

## Adapter-Based Engines (4)

| Engine | Canonical ID | Adapter Files | Standalone Repo | Presets | Seance | Notes |
|--------|-------------|---------------|-----------------|--------:|--------|-------|
| Opera | Opera | OperaAdapter.h/cpp | XOpera | 156 | **8.85** | Additive-vocal Kuramoto; engine #45 |
| Oxytocin | Oxytocin | OxytocinAdapter.h/cpp | XOxytocin | 130 | **9.5 est.** | Circuit × Love; FLEET LEADER |
| Outwit | Outwit | XOutwitAdapter.h/cpp | XOutwit | 593 | **8.7** | Cellular automata stepsynth |
| Overlap | Overlap | XOverlapAdapter.h/cpp | XOverlap | 577 | **8.4** | KnotMatrix FDN; Real DSP |

---

## Fleet Health Summary

| Metric | Value |
|--------|-------|
| **Total engines** | 73 |
| **Total presets** | 17,251 (13,260 non-quarantine + 3,991 quarantine) |
| **Engines with 8.5+ score** | 23 confirmed V1-ready |
| **Engines with 9.0+ score** | 3 (Oxbow 9.0, Oxytocin 9.5 est., Opcode 9.0) |
| **Engines with seance** | 71 (2 NOT SEANCED: Osmosis, Outlook) |
| **Engines with retreat docs** | 52 |
| **Engines with 100+ presets** | 47 |
| **Engines with <50 presets** | 26 (mostly Kitchen Collection; thin coverage) |
| **Fleet median score (numeric)** | 8.5/10 |
| **Fleet average (estimated)** | 8.2/10 |

---

## Critical Fleet Issues (P0 Blockers)

| Issue | Engines | Impact | Path to Resolution |
|-------|---------|--------|-------------------|
| **BROTH Coordinator Not Written** | Overwash, Overworn, Overflow, Overcast | Cross-engine flavor chemistry inert | Write `setBrothSessionAge()` + getter methods in XOceanusProcessor.cpp |
| **P0 DSP Dead Parameters** | Ogre (soil), Ochre (LFO2), Obelisk (3), Opaline (coupling), Octave (2), Oware (3 LFO2 params) | 7 engines fail D004 (Dead Params Are Broken Promises) | Fix per-engine, re-seance |
| **Osmosis Design-Phase Only** | Osmosis | 1 preset, never seanced | Seance the design (or defer to V1.1) |
| **Outlook New Engine** | Outlook | 18 presets, never seanced | Formal seance required before shipping |
| **Leslie Doppler Wrong** | Otis | Amplitude modulation instead of pitch | Fix to pitch-mode Doppler; re-seance |
| **Kitchen Preset Thin Coverage** | All 24 Kitchen engines | Max 34 presets per engine; need 100+ | Batch preset expansion campaign |
| **Overgrow Bow Noise Bug** | Overgrow | Bow noise added to output, not delay line | Fix impulse routing; re-seance |

---

## V1 Readiness (Confirmed 23 Engines)

**Ready for ship** (8.5+ seance OR approved + 100+ presets + no P0 blockers):

1. Obrix (9.4 roadmap) — FLAGSHIP
2. Obscura (HIGH/unanimous)
3. Offering (8.8)
4. Onset (Ahead of industry)
5. Opal (APPROVED)
6. Opera (8.85)
7. Optic (Revolutionary)
8. Oracle (8.6)
9. Orbital (APPROVED)
10. Orca (~8.6)
11. Organon (8/8 PASS)
12. Origami (APPROVED)
13. Orphica (~8.7)
14. Osprey (APPROVED)
15. Osteria (Production-grade)
16. Ostinato (8.7)
17. Oto (8.6 post-fix)
18. Ouie (8.5)
19. Ouroboros (Production-ready)
20. Outwit (8.7)
21. Oware (8.7 → 9.2 projected)
22. Oxbow (9.0)
23. Oxytocin (9.5 est.)

---

## Recommended Actions

### Immediate (P0)
1. Write BROTH Coordinator (unblocks 4 engines)
2. Fix 7 P0 DSP parameter bugs (Ogre, Ochre, Obelisk, Opaline, Octave, Oware LFO2)
3. Seance Osmosis and Outlook

### Before V1 Ship (P1)
1. Formal numeric re-seance for all 5 legacy aliases (Overbite, Oblong, Obese, OddOscar, OddfeliX)
2. Retreat docs for Oxytocin (fleet leader, no retreat yet), Ouie, OddfeliX, OddOscar
3. Kitchen Collection preset expansion (all engines need 100+ presets)
4. Re-seance Organism and Overtone (critical DSP fixes applied 2026-03-20)

### V1.1 (P2)
1. Formal numeric seance for all non-numeric approvals (11 engines)
2. Leslie Doppler fix for Otis
3. isHusband regression fix for Ole
4. OceanDeep filter ADSR addition

---

*Inventory generated 2026-03-28. Ground truth sourced from:*
- *Source/Engines/ filesystem (73 directories, 502+ files)*
- *Source/Core/EngineRegistry.h (canonical engine list)*
- *Presets/XOceanus/ (17,251 presets, JSON-parsed)*
- *Docs/fleet-health-dashboard-2026-03-24.md (seance scores, retreat status)*
