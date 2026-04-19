# XOceanus — Claude Code Project Guide

> **Rename note:** Formerly XOmnibus. Renamed to XOceanus 2026-03-24. Motto: "XOceanus — for all."

## Product Identity

XOceanus ("for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**.
It merges character instruments into one unified creative environment where engines couple, collide,
<<<<<<< HEAD
and mutate into sounds impossible with any single synth. **87 engines implemented (101 in full fleet design)**
— full engine color table + accent colors: `Docs/reference/engine-color-table.md`
=======
and mutate into sounds impossible with any single synth. **<!-- ENGINE_COUNT -->88<!-- /ENGINE_COUNT --> engines implemented (<!-- ENGINE_COUNT_DESIGNED -->111<!-- /ENGINE_COUNT_DESIGNED --> in full fleet design)**
— single source of truth: `Docs/engines.json` · color table: `Docs/reference/engine-color-table.md`
>>>>>>> 5c65e0bd133b4e1a479f36275ebd62628cf46341

- **Coupling:** Cross-engine modulation via MegaCouplingMatrix (15 coupling types incl. KnotTopology + TriangularCoupling)
- **PlaySurface:** 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Presets:** 19,859 active factory presets in `.xometa` format, 16 mood categories (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic, Shadow), 6D Sonic DNA
- **Formats:** AU, Standalone (macOS); AUv3, Standalone (iOS); VST3 (v2)
- **Design:** Gallery Model — dark mode default. Engine accent colors frame the interface.

## Brand Rules

- All XO_OX instruments follow the **XO + O-word** naming convention
- Character over feature count — every feature must support a sonic pillar
- Dry patches must sound compelling before effects are applied
- Presets are a core product feature, not an afterthought
- Dark mode is the primary presentation. Light mode is available as a toggle.

## Architecture Rules

- **Never** allocate memory on the audio thread
- **Never** perform blocking I/O on the audio thread
- **Never** rename stable parameter IDs after release
- All DSP lives inline in `.h` headers; `.cpp` files are one-line stubs
- DSP modules must be testable in isolation
- Export systems must run on non-audio worker threads
- Denormal protection required in all feedback/filter paths
- Engine hot-swap uses 50ms crossfade to prevent clicks
- ParamSnapshot pattern: cache all parameter pointers once per block

## The 6 Doctrines

| ID | Doctrine | Summary |
|----|----------|---------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — not just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Rigor and citation required for any physically-modeled engine |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | Every engine needs at least one LFO with rate floor ≤ 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch / mod wheel / expression) |

## Engine ID → Parameter Prefix

Engine IDs (used in preset `"engines"` arrays, `"parameters"` keys, UI, and coupling routes)
were renamed to O-prefix convention. **Parameter prefixes are frozen and never change:**

| Engine ID | Parameter Prefix | Example |
|-----------|-----------------|---------|
| OddfeliX | `snap_` | `snap_filterCutoff` |
| OddOscar | `morph_` | `morph_morph` |
| Overdub | `dub_` | `dub_oscWave` |
| Odyssey | `drift_` | `drift_oscA_mode` |
| Oblong | `bob_` | `bob_fltCutoff` |
| Obese | `fat_` | `fat_satDrive` |
| Overbite | `poss_` | `poss_biteDepth` |
| Onset | `perc_` | `perc_noiseLevel` |
| Overworld | `ow_` | `ow_era` |
| Opal | `opal_` | `opal_grainSize` |
| Orbital | `orb_` | `orb_brightness` |
| Organon | `organon_` | `organon_metabolicRate` |
| Ouroboros | `ouro_` | `ouro_topology` |
| Obsidian | `obsidian_` | `obsidian_depth` |
| Origami | `origami_` | `origami_foldPoint` |
| Oracle | `oracle_` | `oracle_breakpoints` |
| Obscura | `obscura_` | `obscura_stiffness` |
| Oceanic | `ocean_` | `ocean_separation` |
| Ocelot | `ocelot_` | `ocelot_biome` |
| Optic | `optic_` | `optic_pulseRate` |
| Oblique | `oblq_` | `oblq_prismColor` |
| Osprey | `osprey_` | `osprey_shoreBlend` |
| Osteria | `osteria_` | `osteria_qBassShore` |
| Owlfish | `owl_` | `owl_filterCutoff` |
| Ohm | `ohm_` | `ohm_macroMeddling` |
| Orphica | `orph_` | `orph_pluckBrightness` |
| Obbligato | `obbl_` | `obbl_breathA` |
| Ottoni | `otto_` | `otto_macroGrow` |
| Ole | `ole_` | `ole_macroDrama` |
| Ombre | `ombre_` | `ombre_blend` |
| Orca | `orca_` | `orca_huntMacro` |
| Octopus | `octo_` | `octo_armDepth` |
| Ostinato | `osti_` | `osti_macroGather` |
| OpenSky | `sky_` | `sky_macroRise` |
| OceanDeep | `deep_` | `deep_macroPressure` |
| Ouie | `ouie_` | `ouie_macroHammer` |
| Overlap | `olap_` | `olap_knotDepth` |
| Outwit | `owit_` | `owit_armDepth` |
| Obrix | `obrix_` | `obrix_src1Type` |
| Orbweave | `weave_` | `weave_knotDepth` |
| Overtone | `over_` | `over_cfDepth` |
| Organism | `org_` | `org_ruleSet` |
| Oxbow | `oxb_` | `oxb_entangle` |
| Oware | `owr_` | `owr_material` |
| Opera | `opera_` | `opera_drama` |
| Offering | `ofr_` | `ofr_digCuriosity` |
| Osmosis | `osmo_` | `osmo_permeability` |
| Oxytocin | `oxy_` | `oxy_intimacy` |
| Oto | `oto_` | `oto_drawbar1` |
| Octave (Chef) | `oct_` | `oct_tonewheel` |
| Oleg | `oleg_` | `oleg_harmonics` |
| Otis | `otis_` | `otis_soulDrive` |
| Oven | `oven_` | `oven_hammerBrightness` |
| Ochre | `ochre_` | `ochre_woodResonance` |
| Obelisk | `obel_` | `obel_stringMass` |
| Opaline | `opal2_` | `opal2_preparedDensity` |
| Ogive | `ogv_` | `ogv_tracery_stiff` |
| Olvido | `olv_` | `olv_depth` |
| Ogre | `ogre_` | `ogre_subFreq` |
| Olate | `olate_` | `olate_fretlessGlide` |
| Oaken | `oaken_` | `oaken_stringAge` |
| Omega | `omega_` | `omega_synthBass` |
| Orchard | `orch_` | `orch_bowPressure` |
| Overgrow | `grow_` | `grow_growthRate` |
| Osier | `osier_` | `osier_windDensity` |
| Oxalis | `oxal_` | `oxal_leafTension` |
| Overwash | `wash_` | `wash_tideDepth` |
| Overworn | `worn_` | `worn_feltAge` |
| Overflow | `flow_` | `flow_currentSpeed` |
| Overcast | `cast_` | `cast_cloudDensity` |
| Oasis | `oas_` | `oas_springDepth` |
| Oddfellow | `oddf_` | `oddf_spectralShift` |
| Onkolo | `onko_` | `onko_resonanceCore` |
| Opcode | `opco_` | `opco_codeDepth` |
| Outlook | `look_` | `look_horizonScan` |
| Obiont | `obnt_` | `obnt_evolutionRate` |
| Okeanos | `okan_` | `okan_warmth` |
| Outflow | `out_` | `out_currentSpeed` |
| Oxidize | `oxidize_` | `oxidize_ageRate` |
| Onrush | `onr_` | `onr_swellThresh` |
| Omnistereo | `omni_` | `omni_tapeAge` |
| Obliterate | `oblt_` | `oblt_shimmerDecay` |
| Obscurity | `obsc_` | `obsc_pllGlide` |
| Oubliette | `oubl_` | `oubl_scanRate` |
| Osmium | `osmi_` | `osmi_meat30hz` |
| Orogen | `orog_` | `orog_ringFreq` |
| Oculus | `ocul_` | `ocul_voltageFreq` |
| Outage | `outg_` | `outg_crushBits` |
| Override | `ovrd_` | `ovrd_overdrive` |
| Occlusion | `occl_` | `occl_freqShift` |
| Obdurate | `obdr_` | `obdr_ufGain` |
| Orison | `oris_` | `oris_shimmerSize` |
| Overshoot | `ovsh_` | `ovsh_transThresh` |
| Obverse | `obvr_` | `obvr_mirrorMix` |
| Oxymoron | `oxym_` | `oxym_contrastAmt` |
| Ornate | `orna_` | `orna_crystalSize` |
| Oration | `orat_` | `orat_semi0` |
| Offcut | `offc_` | `offc_gateThresh` |
| Omen | `omen_` | `omen_spectralFreeze` |
| Opus | `opus_` | `opus_oscillatorMode` |
| Outlaw | `outl_` | `outl_diodeClip` |
| Outbreak | `outb_` | `outb_virusFilter` |
| Orrery | `orry_` | `orry_planetSpeed` |
<<<<<<< HEAD
| Observandum | `observ_` | `observ_curveMorph` |
| Opsin | `ops_` | `ops_topology` |
| Oort | `oort_` | `oort_scatter` |
| Ondine | `ond_` | `ond_formant` |
| Ortolan | `ort_` | `ort_pulse` |
| Octant | `octn_` | `octn_tensor` |
| Overtide | `ovt_` | `ovt_wavelet` |
| Oobleck | `oobl_` | `oobl_feed` |
| Ooze | `ooze_` | `ooze_reynolds` |
=======
| Observandum | `observ_` | `observ_scanOffset` |
| Oort | `oort_` | `oort_cloudDensity` |
| Opsin | `ops_` | `ops_photonFlux` |
| Octant | `octn_` | `octn_bearing` |
| Ondine | `ond_` | `ond_driftRate` |
| Ortolan | `ort_` | `ort_songPhase` |
| Ostracon | `ostr_` | `ostr_oxide` |
| Overtide | `ovt_` | `ovt_tidalDepth` |
>>>>>>> 5c65e0bd133b4e1a479f36275ebd62628cf46341

Legacy engine names (`Snap`, `Morph`, `Dub`, `Drift`, `Bob`, `Fat`, `Bite`)
are resolved automatically by `resolveEngineAlias()` in `PresetManager.h`.
See `Docs/specs/xoceanus_name_migration_reference.md` for the full mapping and gotchas.

## Key Files

| Path | Purpose |
|------|---------|
| `Docs/specs/xoceanus_master_specification.md` | **THE** single source of truth |
| `Docs/specs/xoceanus_name_migration_reference.md` | Legacy → canonical engine name mapping |
| `Docs/engines.json` | **Single source of truth** for engine roster + count. Edit here; run `python Tools/sync_engine_sources.py` to propagate. |
| `Docs/reference/engine-color-table.md` | Full engine color table + Blessings + Debates (<!-- ENGINE_COUNT -->88<!-- /ENGINE_COUNT --> implemented, <!-- ENGINE_COUNT_DESIGNED -->111<!-- /ENGINE_COUNT_DESIGNED --> fleet design) |
| `Source/Core/SynthEngine.h` | Engine interface (all engines implement this) |
| `Source/Core/EngineRegistry.h` | Factory + 4-slot management |
| `Source/Core/MegaCouplingMatrix.h` | Cross-engine modulation |
| `Source/Core/PresetManager.h` | .xometa loading/saving |
| `Source/Core/EpicChainSlotController.h` | Unified 3-slot FX assignment with 50ms crossfade |
| `Source/Engines/{Name}/{Name}Engine.h` | All engine adapters (pattern) |
| `Source/DSP/` | Shared DSP library |
| `Source/DSP/Effects/` | FX chains: Aquatic, Math, Boutique, Singularity (fXO*), Gemini Pedalboard, Epic Chains |
| `Source/UI/` | Gallery Model UI components |
| `Source/Export/` | XPN export pipeline |
| `SDK/include/xoceanus/` | JUCE-free SDK headers for third-party engine development |
| `Presets/XOceanus/{mood}/` | Factory presets by mood |
| `Tools/` | Python utilities (DNA, breeding, migration, export) |
| `Source/AI/` | AI sound assistant (gated behind `XOCEANUS_BUILD_AI` CMake option, default OFF) |

## Preset System

- `.xometa` JSON files are the source of truth (version-controlled)
- 16 moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic, Shadow
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- 6D Sonic DNA: brightness, warmth, movement, density, space, aggression
- Naming: 2-3 words, evocative, max 30 chars, no duplicates, no jargon

## Design System

- **Gallery Model:** Warm white shell `#F8F6F3` frames engine accent colors
- **XO Gold:** `#E9C46A` — brand constant (macros, coupling strip, active states)
- **Typography:** Space Grotesk (display), Inter (body), JetBrains Mono (values)
- **Dark mode default**, light mode toggle

## Environment

- **CMake ≥ 3.22**, **Ninja**, **Xcode Command Line Tools** required
- JUCE 8 lives in `Libs/JUCE/` (fetched automatically via FetchContent if absent)
- AU plugin code: `aumu Xocn XoOx` — used for auval and DAW identification
- `OSX_ARCHITECTURES` must be set **before** `project()` in CMakeLists.txt (universal binary)
- Use `apvts.processor` not `getProcessor()` in JUCE 8
- `atomic.load()` required for jmax; `1`-param `keyPressed` override

## Build

```bash
# macOS (Release)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# macOS (Debug)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# iOS
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=cmake/ios-toolchain.cmake \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID
cmake --build build-ios --config Release

# Validate AU (run after every macOS build)
auval -v aumu Xocn XoOx
```

## XPN Export (MPC Compatibility)

3 critical XPM rules — no exceptions:
- `KeyTrack` = `True` (samples transpose across zones)
- `RootNote` = `0` (MPC auto-detect convention)
- Empty layer `VelStart` = `0` (prevents ghost triggering)

## Adding New Engines

New engines are designed as standalone instruments first, then integrated into XOceanus.

**Invoke:** `/new-xo-engine` — walks through ideation, architecture, scaffold, and integration prep.

**Full process:** `Docs/specs/xoceanus_new_engine_process.md`

**Quick rules for XOceanus-ready standalone development:**
- Parameter IDs use `{shortname}_{paramName}` format from day one
- Presets use `.xometa` JSON format from day one
- DSP lives in inline `.h` headers (portable)
- DSP works without UI references (just parameters)
- M1-M4 macros produce audible change in every preset
- Define coupling compatibility early (which `CouplingType` enums you accept)

**Integration path:** Write a thin adapter implementing `SynthEngine` → `REGISTER_ENGINE()` → copy presets → done.

### CLAUDE.md Checklist (when registering a new engine)

Update **all four** of these sections in CLAUDE.md:

1. **Product Identity header** — update the engine count
2. **Engine color table** (`Docs/reference/engine-color-table.md`) — add a row
3. **Parameter Prefix table** (above) — add a row
4. **Key Files table** (if the engine has a notable architecture worth linking)

Then update these external files:
- `Docs/specs/xoceanus_master_specification.md` section 3.1 engine table (add row)
- `Docs/seances/seance_cross_reference.md` (add seance row after the seance is run)
- `Source/XOceanusProcessor.cpp` (register the engine)
- `Source/Core/PresetManager.h` (add to `validEngineNames` and `frozenPrefixForEngine`)

Full process: `Docs/specs/xoceanus_new_engine_process.md`

## Release Philosophy — "The Deep Opens"

XOceanus does **not** operate on a fixed release cutoff. Build and refine until it's ready; ship when it's ready. There is no "V1 scope", no feature freeze, no curated subset gating a launch. The full <!-- ENGINE_COUNT_DESIGNED -->111<!-- /ENGINE_COUNT_DESIGNED -->-engine fleet design is the long-arc target; <!-- ENGINE_COUNT -->88<!-- /ENGINE_COUNT --> engines are implemented to date.

Do not propose "V1 readiness" plans, "V1 candidate" lists, or "ship V1" timelines. If a Claude session generates a cutoff-style roadmap, it is off-brief — correct it.

**Patreon milestone unlocks:** Kitchen Collection quads released at patron thresholds (10/25/50/100/250/500). Permanent free once unlocked. (These are release milestones tied to patron count, not a version gate.)

### Kitchen Collection (24 engines across 6 quads)

All 6 quads built, seanced, and Guru Bin retreats complete (2026-03-23):

| Quad | Theme | Engines | Status |
|------|-------|---------|--------|
| Chef (Organs) | Adversarial coupling | OTO, OCTAVE, OLEG, OTIS | Complete |
| Kitchen (Pianos) | Modal resonator banks | OVEN, OCHRE, OBELISK, OPALINE | Complete |
| Cellar (Bass) | Gravitational coupling | OGRE, OLATE, OAKEN, OMEGA | Complete |
| Garden (Strings) | Growth Mode | ORCHARD, OVERGROW, OSIER, OXALIS | Complete |
| Broth (Pads) | Multi-timescale diffusion | OVERWASH, OVERWORN, OVERFLOW, OVERCAST | Complete |
| Fusion (EP) | Spectral Fingerprint Cache | OASIS, ODDFELLOW, ONKOLO, OPCODE | Complete |

## Fleet Quality Summary

- **Fleet seanced:** see `Docs/engines.json` for authoritative per-engine status; fleet average ~8.8/10. Six engines at 9.0+ (OXYTO 9.5, OVERBITE 9.2, OWARE 9.2, OBSCURA 9.1, OUROBOROS 9.0, OXBOW 9.0). Recent: Observandum 8.6, Orrery 8.7, Opsin 8.7, Oort 8.7 (seanced 2026-04-15). OBIONT 8.2 (D004+D002 fixes applied).
- **All 6 doctrines resolved fleet-wide** (D001–D006). Prism Sweep 12-round pass complete (2026-03-14).
- **49 Blessings** awarded — full table: `Docs/reference/engine-color-table.md`
- **2 Open Debates** (DB001, DB002) — DB003–DB005 resolved. See reference table
- Detailed scores: `Docs/fleet-audit/fleet-seance-scores-2026-03-20.md`
- Seance cross-reference: `Docs/seances/seance_cross_reference.md`
- Individual verdicts: `Docs/seances/` and `scripture/seances/`

---

## Skill Library

Reusable skill guides live in `Skills/` — invoke the relevant one before starting each task type.

| Skill | Invoke | When to Use |
|-------|--------|-------------|
| [coupling-preset-designer](Skills/coupling-preset-designer/SKILL.md) | `/coupling-preset-designer` | Creating Entangled mood presets with cross-engine coupling |
| [coupling-interaction-cookbook](Skills/coupling-interaction-cookbook/SKILL.md) | `/coupling-interaction-cookbook` | Quick lookup: which engines pair best, what types are supported |
| [mod-matrix-builder](Skills/mod-matrix-builder/SKILL.md) | `/mod-matrix-builder` | Adding 8-slot mod matrix to any engine (D002/D005/D006) |
| [preset-architect](Skills/preset-architect/SKILL.md) | `/preset-architect` | Writing any new `.xometa` preset |
| [engine-health-check](Skills/engine-health-check/SKILL.md) | `/engine-health-check` | Quick D001–D006 doctrine check on any engine |
| [dna-designer](Skills/dna-designer/SKILL.md) | `/dna-designer` | Assigning accurate 6D Sonic DNA to presets |
| [xpn-export-specialist](Skills/xpn-export-specialist/SKILL.md) | `/xpn-export-specialist` | Full XPN/MPC export pipeline |
| [midi-daw-audit](Skills/midi-daw-audit/SKILL.md) | `/midi-daw-audit` | MIDI/DAW compatibility audit |
| [sro-optimizer](Skills/sro-optimizer/SKILL.md) | `/sro-optimizer` | CPU optimization via Spectral Resonance Objects |
| [preset-auditor](Skills/preset-auditor/SKILL.md) | `/preset-auditor` | Preset quality gate — DNA, macros, D004, naming |
| [coupling-debugger](Skills/coupling-debugger/SKILL.md) | `/coupling-debugger` | Diagnose broken or inaudible coupling routes |
| [master-audit](Skills/master-audit/SKILL.md) | `/master-audit` | Fleet-wide QA orchestration — pre-release health check |
| [repo-audit](Skills/repo-audit/SKILL.md) | `/repo-audit` | Repo hygiene + doc currency |
| synth-seance (`~/.claude/skills/`) | `/synth-seance` | Ghost council full engine quality evaluation |
| post-engine-completion-checklist (`~/.claude/skills/`) | `/post-engine-completion-checklist` | 5-point post-build audit |
| producers-guild (`~/.claude/skills/`) | `/producers-guild` | 12-specialist market/product review |
| [preset-audit-checklist](Skills/preset-audit-checklist/SKILL.md) | `/preset-audit-checklist` | 7-phase Guru-informed preset audit |
| [new-xo-engine](Skills/new-xo-engine/SKILL.md) | `/new-xo-engine` | End-to-end new engine creation |
| [skill-friction-detective](Skills/skill-friction-detective/SKILL.md) | `/skill-friction-detective` | Meta-skill: detect friction/failures across all skills |
| ringleader (`.claude/skills/`) | `/ringleader` | Strategic leadership — RAC verdicts, battle plans, handoffs, fleet governance, triage |

Full index: `Skills/README.md`

---

## Documentation Index

- Full doc inventory: `Docs/MANIFEST.md`
- Documentation governance: `Docs/GOVERNANCE.md`
- Master specification: `Docs/specs/xoceanus_master_specification.md`
- Discovery index: `Docs/INDEX.md`
- Community strategy: `Docs/plans/community-strategy-v2.md`
- Kitchen Collection release calendar: `Docs/plans/kitchen-collection-release-calendar.md`
- Fleet seance scores: `Docs/fleet-audit/fleet-seance-scores-2026-03-20.md`

---

## Development Workflow

1. Read the master spec (`Docs/specs/xoceanus_master_specification.md`) before making changes
2. Plan before coding — produce architecture + QA plan first
3. All engines must implement the `SynthEngine` interface
4. Parameter IDs are namespaced by engine (e.g., `snap_filterCutoff`, `dub_sendAmount`)
5. Run DSP stability checks after any engine modifications
6. Preserve existing parameter IDs and preset compatibility
7. For new engines, follow the process in `Docs/specs/xoceanus_new_engine_process.md`
