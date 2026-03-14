# XOmnibus — Claude Code Project Guide

## Product Identity

XOmnibus ("for all") is a free, open-source multi-engine synthesizer platform by **XO_OX Designs**.
It merges character instruments into one unified creative environment where engines couple, collide,
and mutate into sounds impossible with any single synth. 31 engines are integrated with DSP code
(5 Constellation family engines added 2026-03-14) — see engine table below.

- **Engine modules:** ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, OVERWORLD, OPAL, ORBITAL, ORGANON, OUROBOROS, OBSIDIAN, OVERBITE, ORIGAMI, ORACLE, OBSCURA, OCEANIC, OCELOT, OPTIC, OBLIQUE, OSTINATO, OPENSKY, OCEANDEEP, OWLFISH, OUIE, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE
- **Coupling:** Cross-engine modulation via MegaCouplingMatrix (12 coupling types)
- **PlaySurface:** 4-zone unified playing interface (Pad/Fretless/Drum modes)
- **Presets:** 2,369 factory presets in `.xometa` format, 7 mood categories (incl. Family), 6D Sonic DNA
- **Formats:** AU, Standalone (macOS); AUv3, Standalone (iOS); VST3 (v2)
- **Design:** Gallery Model — warm white shell frames engine accent colors. Light mode default.

## Brand Rules

- All XO_OX instruments follow the **XO + O-word** naming convention
- Character over feature count — every feature must support a sonic pillar
- Dry patches must sound compelling before effects are applied
- Presets are a core product feature, not an afterthought
- Light mode is the primary presentation. Dark mode is a toggle.

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

## Engine Modules

| Short Name | Source Instrument | Accent Color |
|-----------|------------------|-------------|
| ODDFELIX | OddfeliX (feliX the neon tetra) | Neon Tetra Blue `#00A6D6` |
| ODDOSCAR | OddOscar (Oscar the axolotl) | Axolotl Gill Pink `#E8839B` |
| OVERDUB | XOverdub | Olive `#6B7B3A` |
| ODYSSEY | XOdyssey | Violet `#7B2D8B` |
| OBLONG | XOblong | Amber `#E9A84A` |
| OBESE | XObese | Hot Pink `#FF1493` |
| ONSET | XOnset | Electric Blue `#0066FF` |
| OVERWORLD | XOverworld | Neon Green `#39FF14` |
| OPAL | XOpal | Lavender `#A78BFA` |
| ORGANON | XOrganon | Bioluminescent Cyan `#00CED1` |
| OUROBOROS | XOuroboros | Strange Attractor Red `#FF2D2D` |
| OBSIDIAN | XObsidian | Crystal White `#E8E0D8` |
| ORIGAMI | XOrigami | Vermillion Fold `#E63946` |
| ORACLE | XOracle | Prophecy Indigo `#4B0082` |
| OBSCURA | XObscura | Daguerreotype Silver `#8A9BA8` |
| OCEANIC | XOceanic | Phosphorescent Teal `#00B4A0` |
| OCELOT | XOcelot | Ocelot Tawny `#C5832B` |
| OVERBITE | XOverbite | Fang White `#F0EDE8` |
| ORBITAL | XOrbital | Warm Red `#FF6B6B` |
| OPTIC | XOptic | Phosphor Green `#00FF41` |
| OBLIQUE | XOblique | Prism Violet `#BF40FF` |
| OSTINATO | XOstinato | Firelight Orange `#E8701A` |
| OPENSKY | XOpenSky | Sunburst `#FF8C00` |
| OCEANDEEP | XOceanDeep | Trench Violet `#2D0A4E` |
| OWLFISH | XOwlfish | Abyssal Gold `#B8860B` |
| OUIE | XOuïe | Hammerhead Steel `#708090` |
| OHM | XOhm | Sage `#87AE73` |
| ORPHICA | XOrphica | Siren Seafoam `#7FDBCA` |
| OBBLIGATO | XObbligato | Rascal Coral `#FF8A7A` |
| OTTONI | XOttoni | Patina `#5B8A72` |
| OLE | XOlé | Hibiscus `#C9377A` |

### Engine ID vs Parameter Prefix

Engine IDs (used in preset `"engines"` arrays, `"parameters"` keys, UI, and coupling routes)
were renamed to O-prefix convention. **Parameter prefixes are frozen and never change:**

| Engine ID | Parameter Prefix | Example |
|-----------|-----------------|---------|
| OddfeliX | `snap_` | `snap_filterCutoff` |
| OddOscar | `morph_` | `morph_scanPos` |
| Overdub | `dub_` | `dub_sendAmount` |
| Odyssey | `odyssey_` | `odyssey_detune` |
| Oblong | `bob_` | `bob_fltCutoff` |
| Obese | `fat_` | `fat_satDrive` |
| Overbite | `poss_` | `poss_biteDepth` |
| Onset | `onset_` | `onset_noiseLevel` |
| Overworld | `ow_` | `ow_era` |
| Opal | `opal_` | `opal_grainSize` |
| Orbital | `orbital_` | `orbital_partialTilt` |
| Organon | `organon_` | `organon_entropy` |
| Ouroboros | `ouroboros_` | `ouroboros_feedback` |
| Obsidian | `obsidian_` | `obsidian_pdDepth` |
| Origami | `origami_` | `origami_foldPoint` |
| Oracle | `oracle_` | `oracle_breakpoints` |
| Obscura | `obscura_` | `obscura_stiffness` |
| Oceanic | `oceanic_` | `oceanic_separation` |
| Ocelot | `ocelot_` | `ocelot_biome` |
| Optic | `optic_` | `optic_pulseRate` |
| Oblique | `oblq_` | `oblq_prismColor` |
| Ostinato | `osti_` | `osti_seatN_instrument` |
| OpenSky | `sky_` | `sky_shimmerAmount` |
| OceanDeep | `deep_` | `deep_pressure` |
| Owlfish | `owl_` | `owl_filterCutoff` |
| Ouïe | `ouie_` | `ouie_hammer` |
| Ohm | `ohm_` | `ohm_macroMeddling` |
| Orphica | `orph_` | `orph_pluckBrightness` |
| Obbligato | `obbl_` | `obbl_breathA` |
| Ottoni | `otto_` | `otto_macroGrow` |
| Ole | `ole_` | `ole_macroDrama` |

Legacy engine names (`Snap`, `Morph`, `Dub`, `Drift`, `Bob`, `Fat`, `Bite`)
are resolved automatically by `resolveEngineAlias()` in `PresetManager.h`.
See `Docs/xomnibus_name_migration_reference.md` for the full mapping and gotchas.

## Key Files

| Path | Purpose |
|------|---------|
| `Docs/xomnibus_master_specification.md` | **THE** single source of truth |
| `Docs/xomnibus_name_migration_reference.md` | Legacy → canonical engine name mapping |
| `Source/Core/SynthEngine.h` | Engine interface (all engines implement this) |
| `Source/Core/EngineRegistry.h` | Factory + 4-slot management |
| `Source/Core/MegaCouplingMatrix.h` | Cross-engine modulation |
| `Source/Core/PresetManager.h` | .xometa loading/saving |
| `Source/Engines/*/` | Engine adapter modules |
| `Source/Engines/Optic/OpticEngine.h` | Visual modulation engine + AutoPulse |
| `Source/Engines/Oblique/ObliqueEngine.h` | Prismatic bounce engine (RTJ x Funk x Tame Impala) |
| `Source/UI/OpticVisualizer/OpticVisualizer.h` | Winamp-style audio-reactive visualizer |
| `Docs/xomnibus_sound_design_guides.md` | Sound design guide (20 of 25 engines covered) |
| `Source/DSP/` | Shared DSP library |
| `Source/UI/` | Gallery Model UI components |
| `Source/Export/` | XPN export pipeline |
| `Presets/XOmnibus/{mood}/` | Factory presets by mood |
| `Tools/` | Python utilities (DNA, breeding, migration, export) |
| `Docs/` | All specification documents |

## Preset System

- `.xometa` JSON files are the source of truth (version-controlled)
- 6 moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether
- 4 macros: CHARACTER, MOVEMENT, COUPLING, SPACE
- 6D Sonic DNA: brightness, warmth, movement, density, space, aggression
- Naming: 2-3 words, evocative, max 30 chars, no duplicates, no jargon

## Design System

- **Gallery Model:** Warm white shell `#F8F6F3` frames engine accent colors
- **XO Gold:** `#E9C46A` — brand constant (macros, coupling strip, active states)
- **Typography:** Space Grotesk (display), Inter (body), JetBrains Mono (values)
- **Light mode default**, dark mode toggle

## Build

```bash
# macOS build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# iOS build
cmake -B build-ios -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=ios-toolchain.cmake \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID
cmake --build build-ios --config Release
```

## XPN Export (MPC Compatibility)

3 critical XPM rules — no exceptions:
- `KeyTrack` = `True` (samples transpose across zones)
- `RootNote` = `0` (MPC auto-detect convention)
- Empty layer `VelStart` = `0` (prevents ghost triggering)

## Adding New Engines

New engines are designed as standalone instruments first, then integrated into XOmnibus.

**Invoke:** `/new-xo-engine` — walks through ideation, architecture, scaffold, and integration prep.

**Full process:** `Docs/xomnibus_new_engine_process.md`

**Quick rules for XOmnibus-ready standalone development:**
- Parameter IDs use `{shortname}_{paramName}` format from day one
- Presets use `.xometa` JSON format from day one
- DSP lives in inline `.h` headers (portable)
- DSP works without UI references (just parameters)
- M1-M4 macros produce audible change in every preset
- Define coupling compatibility early (which `CouplingType` enums you accept)

**Integration path:** Write a thin adapter implementing `SynthEngine` → `REGISTER_ENGINE()` → copy presets → done.

## Seance Findings

29 seances complete (2026-03-14) — all 31 engines covered (26 original + 5 Constellation). Constellation seances (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) completed same day; findings committed 836e85a. Full data in:
- Grand Survey: `Docs/xomnibus_landscape_2026.md`
- Knowledge tree: `~/.claude/skills/synth-seance/knowledge/index.md`
- Cross-reference: `Docs/seance_cross_reference.md`

### The 6 Doctrines

| ID | Doctrine | Summary |
|----|----------|---------|
| D001 | Velocity Must Shape Timbre | Velocity drives filter brightness / harmonic content — not just amplitude |
| D002 | Modulation is the Lifeblood | Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots |
| D003 | The Physics IS the Synthesis | Rigor and citation required for any physically-modeled engine |
| D004 | Dead Parameters Are Broken Promises | Every declared parameter must affect audio output |
| D005 | An Engine That Cannot Breathe Is a Photograph | Every engine needs at least one LFO with rate floor ≤ 0.01 Hz |
| D006 | Expression Input Is Not Optional | Velocity→timbre + at least one CC (aftertouch / mod wheel / expression) |

### The 15 Blessings

| ID | Blessing | Engine |
|----|----------|--------|
| B001 | Group Envelope System — crowned by Moog + Smith | ORBITAL |
| B002 | XVC Cross-Voice Coupling — all 8 ghosts, 3-5 years ahead | ONSET |
| B003 | Leash Mechanism — chaotic system with a leash | OUROBOROS |
| B004 | Spring Reverb — Vangelis + Tomita praised the metallic splash | OVERDUB |
| B005 | Zero-Audio Identity — synthesis without sound | OPTIC |
| B006 | Dual-Layer Blend Architecture — Circuit + Algorithm crossfade | ONSET |
| B007 | Velocity Coupling Outputs — velocity as coupling signal | OUROBOROS |
| B008 | Five-Macro System (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) — all 8 ghosts | OVERBITE |
| B009 | ERA Triangle — 2D timbral crossfade, Buchla/Schulze/Vangelis/Pearlman | OVERWORLD |
| B010 | GENDY Stochastic Synthesis + Maqam — Buchla gave 10/10 | ORACLE |
| B011 | Variational Free Energy Metabolism — unanimous; publishable as paper | ORGANON |
| B012 | ShoreSystem — 5-coastline cultural data shared across engines | OSPREY + OSTERIA |
| B013 | Chromatophore Modulator — praised by Buchla, Schulze, Kakehashi, Tomita | OCEANIC |
| B014 | Mixtur-Trautonium Oscillator — unanimous praise, genuinely novel | OWLFISH |
| B015 | Mojo Control — orthogonal analog/digital axis | OBESE |

### The 4 Ongoing Debates

| ID | Tension | Status |
|----|---------|--------|
| DB001 | Mutual exclusivity vs. effect chaining (Buchla vs. field) | UNRESOLVED |
| DB002 | Silence as paradigm vs. accessibility (Schulze/Buchla vs. Kakehashi/Pearlman) | UNRESOLVED |
| DB003 | Init patch: immediate beauty vs. blank canvas (Vangelis/Kakehashi vs. Schulze) | UNRESOLVED |
| DB004 | Expression vs. Evolution: gesture vs. temporal depth (Vangelis vs. Schulze) | UNRESOLVED |

### Critical Fleet-Wide Findings

- **Seance score range**: 7.2/10 est. (OBLIQUE, recovered) to 8/8 PASS + 8.6/10 (ORGANON, ORACLE)
- **Preset expansion ongoing**: all engines now have at least 1 preset; thin coverage engines expanded in Rounds 8–11
- **D006 aftertouch coverage**: 22/23 engines have aftertouch (Optic exempt as visual engine)
- **D006 mod wheel coverage**: **22/22 MIDI-capable engines — FULLY RESOLVED** (Round 12C completed the last 7 engines)
- **D001 filter envelopes**: RESOLVED — all engines fleet-wide have velocity-scaled filter envelopes (Round 9E)
- **D004 dead params**: RESOLVED — all declared parameters wired to DSP (Round 3B)
- **D005 LFO breathing**: RESOLVED — all engines have autonomous modulation with rate floor ≤ 0.01 Hz (Round 5A + engine recoveries)

### Prism Sweep Status (2026-03-14)

The Prism Sweep is a 12-round progressive quality pass initiated after all 24 seances. **ALL 12 ROUNDS COMPLETE.** Master index: `Docs/prism_sweep_index.md`

**Completed fixes (Rounds 1–7):**
- ✅ **5 P0 bugs fixed** (Obsidian R-channel, Obsidian formant ID, Osteria warmth L-only, Origami STFT guard, Overworld coupling output) — `Docs/p0_fixes_applied.md`
- ✅ **5 D004 dead params wired** (Snap macroDepth, Owlfish morphGlide, Oblique percDecay, Ocelot 4 macros, Osprey dead LFO) — `Docs/d004_fixes_applied.md`
- ✅ **D005 count 4→0** (Snap 0.15Hz BPF drift, Orbital 0.03Hz morph drift, Overworld ERA drift via eraDriftRate, Owlfish 0.05Hz grain LFO) — `Docs/d005_fixes_applied.md`
- ✅ **2 coupling bugs fixed** (Snap AmpToFilter handler, OPAL per-sample output cache) — `Docs/coupling_fixes_5c.md`
- ✅ **Preset schema migration**: 450+ presets updated across Drift/Bob/Dub/Overworld — `Docs/preset_schema_migration_5e.md`
- ✅ **V007 Climax proven**: 10 Journey Demo presets in `Presets/Drift/Climax/` — `Docs/v007_journey_demo_report.md`
- ✅ **74 preset names elevated** to evocative/poetic vocabulary across 8 engines — `Docs/preset_naming_elevation.md`
- ✅ **3 zero-macro engines recovered** (Overworld, Morph, Oblique — each 0/10 → 8/10) — `Docs/macro_audit.md`
- ✅ **4 filter envelopes added** (Snap, Morph, Oblique new params; Dub default raised) — `Docs/filter_envelope_audit.md`
- ✅ **D006 aftertouch: 10/23 engines** (5 Round 5D + 5 Round 6) — `Docs/d006_aftertouch_fixes.md`
- ✅ **D006 mod wheel: ~9/23 engines** (7 newly wired in Round 7A) — `Docs/d006_modwheel_fixes.md`
- ✅ **AudioToBuffer coupling type** added to core + `AudioRingBuffer.h` implemented — `Docs/audio_to_buffer_implementation.md`
- ✅ **ShoreSystem formally spec'd** (5-coastline shared cultural data, OSPREY+OSTERIA) — `Docs/shore_system_spec.md`
- ✅ **8 major guides/specs written** (Oracle, Organon, ShoreSystem, naming, mod wheel, filter envelopes, macros, AudioToBuffer, sonic DNA)
- ✅ **12 Sonic DNA gap-fill presets** (Optic all 3 gaps resolved, Oblique all 3 gaps resolved) — `Docs/sonic_dna_audit.md`

**Completed fixes (Round 8):**
- ✅ **OBLIQUE deep recovery** (score 5.9→7.2 est.): second D005 LFO added, D001 velocity→fold wiring, preset count 6→20 — `Docs/oblique_deep_recovery.md`
- ✅ **OCELOT deep recovery**: Ecosystem Matrix documented, D005 LFO added, preset count 4→12+ — `Docs/ocelot_deep_recovery.md`
- ✅ **Coupling preset library**: 18 new coupling presets across 6 engine pairs × 3 intensity levels (ONSET→OVERBITE, OPAL→OVERDUB, ODYSSEY→OPAL, OVERWORLD→OPAL, ORACLE→ORGANON, OUROBOROS→ONSET) — `Docs/coupling_preset_library.md`
- ✅ **Init patch improvements**: 4 init patches created (Overworld, Ocelot, Obsidian, Origami) — `Docs/init_patch_improvements.md`
- ✅ **Sonic DNA backfill**: 15 XOwlfish presets received DNA blocks; fleet 1,679/1,679 (100%) — `Docs/sonic_dna_backfill.md`
- ✅ **Build verification**: Main XOmnibus build PASS; 4 XPNExporter pre-existing errors noted — `Docs/build_verification_8h.md`

**Completed fixes (Round 9):**
- ✅ **OBSIDIAN deep recovery** (score 6.6→8.2 est.): formant breathing LFO (0.1Hz), velocity→PD depth, 8 inaugural presets (first-ever OBSIDIAN presets in fleet) — `Docs/obsidian_deep_recovery.md`
- ✅ **XPNExporter build fix**: `StringArray.isEmpty()` API corrected, test target link libraries fixed — 4 pre-existing errors resolved — `Docs/build_verification_8h.md`
- ✅ **Prefix audit**: 1 rogue `org_` preset corrected to `organon_`; fleet 100% compliant — `Docs/organon_prefix_audit.md`
- ✅ **Voice management audit**: Morph Poly/Mono/Legato + glide added, Overworld 5ms crossfade on steal, Ocelot click-on-steal fix — `Docs/voice_management_audit.md`
- ✅ **Filter envelopes fleet-wide** (6 more engines): Orbital, Owlfish, Overworld, Ocelot, Osteria, Osprey — D001 RESOLVED for entire fleet — `Docs/filter_envelope_expansion_9e.md`
- ✅ **D006 aftertouch batch 3** (Overworld, Owlfish, Ocelot, Osprey, Osteria → 15/23 total) — `Docs/d006_aftertouch_fixes.md`
- ✅ **Preset expansion**: Oracle/Overworld/OCELOT/Optic +40 presets; OBSIDIAN 0→8 (last zero-preset engine closed) — `Docs/preset_expansion_9g.md`
- ✅ **Parameter curves**: skewFactor fixes for Snap decay, Morph decay, Ocelot creature envelopes — `Docs/parameter_curve_audit.md`

**Completed fixes (Round 10):**
- ✅ **Deep documentation**: Obscura synthesis guide (46k), Optic synthesis guide (33k), Ouroboros guide (30k) — `Docs/obscura_synthesis_guide.md`, `Docs/optic_synthesis_guide.md`, `Docs/ouroboros_guide.md`
- ✅ **XVC demo presets**: 11 ONSET drum kits demonstrating Cross-Voice Coupling — `Docs/onset_xvc_demo_guide.md`
- ✅ **Bob aggression expansion**: 10 new high-drive presets closing Oblong aggression gap — `Docs/bob_aggression_expansion.md`
- ✅ **Drift FX gap analysis**: Architectural debt documented — 1,353 standalone FX params not exposed in DriftEngine adapter — `Docs/drift_fx_gap_analysis.md`
- ✅ **D006 aftertouch batch 4** (Bob, Bite, Drift, Onset, Opal → 21/23 total) — `Docs/d006_aftertouch_fixes.md`

**Completed fixes (Round 11):**
- ✅ **D006 aftertouch: 22/23 complete** (Ouroboros pre-wired/documented; Obscura forward-ref bug fixed) — `Docs/d006_aftertouch_fixes.md`
- ✅ **Drift Option B**: TidalPulse + Fracture + Reverb DSP ported from XOdyssey standalone (38→45 params; BREATHE + FRACTURE macros now have real DSP) — `Docs/drift_option_b_implementation.md`
- ✅ **AudioToBuffer Phase 2**: OpalEngine scaffold — 4 input slots, `opal_externalMix`, `processAudioRoute()` complete — `Docs/audio_to_buffer_phase2.md`
- ✅ **Oblique legato bug fixed**: unconditional glide → proper `wasAlreadyActive` gate — `Docs/voice_mode_completion_11d.md`
- ✅ **Orbital voiceMode added**: `orb_voiceMode` (Poly/Mono/Legato) with additive partial phase continuity in legato — `Docs/voice_mode_completion_11d.md`
- ✅ **Mod wheel: 9→15/22 engines** (ONSET/OPAL/ORGANON/OUROBOROS/OBSCURA/OWLFISH) — `Docs/d006_modwheel_completion_11e.md`
- ✅ **20 new presets**: 10 Ouroboros (all 4 attractor topologies) + 10 Obscura (first-ever Obscura presets in fleet) — `Docs/ouroboros_obscura_preset_expansion.md`
- ✅ **18 preset renames**: Bob aggression names elevated (Rubber Spine, Steel Rain, Midnight Weight etc.), XVC kits renamed (Neural Storm, Solar System, Gravity Bend etc.) — `Docs/preset_quality_pass_11h.md`
- ✅ **DNA fleet audit**: 1805 presets, 100% coverage, health score 88/100, 7 gap-fill presets for Obscura/Obsidian/Osprey — `Docs/sonic_dna_validation_11i.md`
- ✅ **BUILD PASS + auval PASS**: 5 forward-reference bugs fixed (atPressure scope in Bob/Bite/Onset/Opal/Ouroboros) — `Docs/build_verification_11j.md`

**Completed fixes (Rounds 12A–12L — final round):**
- ✅ **DNA gap fills**: 8 gap presets for XOwlfish/Obese/OddOscar/Oracle/Osteria; health score 88→~92/100 — `Docs/dna_gap_fill_12a.md`
- ✅ **Duplicate cleanup**: 57 duplicate names resolved + 313 underscore-field violations fixed; 1,839 presets, **0 duplicates** — `Docs/duplicate_cleanup_12b.md`
- ✅ **D006 mod wheel: 15→22/22 engines COMPLETE** (Bob/Bite/Dub/Oceanic/Ocelot/Overworld/Osprey) — **D006 FULLY RESOLVED** — `Docs/d006_modwheel_completion_12c.md`
- ✅ **AudioToBuffer Phase 3 spec**: `IAudioBufferSink` interface, DFS cycle detection, FREEZE state machine fully specified — `Docs/audio_to_buffer_phase3_spec.md`
- ✅ **New coupling pairs**: OBSCURA→ORGANON, ONSET→ORGANON, OVERWORLD→OBSCURA — 6 new presets — `Docs/coupling_expansion_12e.md`
- ✅ **Mood distribution**: zero missing engine-mood combinations across all 20 active engines — `Docs/mood_distribution_audit_12f.md`
- ✅ **XVC advanced guide**: 6 new expert kits (17 total), architecture deep-dive — `Docs/onset_xvc_demo_guide.md` (v1.1)
- ✅ **Knowledge tree final update**: doctrine resolution status, CTX-003, blessing implementation notes — `Docs/knowledge_tree_update_12i.md`
- ✅ **BUILD PASS + auval PASS**: all Round 12 source changes verified clean — `Docs/build_verification_12j.md`
- ✅ **Release readiness gate**: **READY FOR RELEASE** — 22 production engines feature-complete, doctrine-compliant — `Docs/release_readiness_12k.md`
- ✅ **Prism Sweep final report**: ~5,500-word sweep narrative — `Docs/prism_sweep_final_report.md`

---

## Development Workflow

1. Read the master spec (`Docs/xomnibus_master_specification.md`) before making changes
2. Plan before coding — produce architecture + QA plan first
3. All engines must implement the `SynthEngine` interface
4. Parameter IDs are namespaced by engine (e.g., `snap_filterCutoff`, `dub_sendAmount`)
5. Run DSP stability checks after any engine modifications
6. Preserve existing parameter IDs and preset compatibility
7. For new engines, follow the process in `Docs/xomnibus_new_engine_process.md`
