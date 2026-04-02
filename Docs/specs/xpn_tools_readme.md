# XPN Tools — Comprehensive Index

All Python scripts in `Tools/` for the XOceanus preset fleet.
**Total scripts: 338** (as of 2026-03-16)

---

## Legend

| Field | Meaning |
|-------|---------|
| Input | What files/dirs it reads |
| Output | What it writes / where |
| Status | `active` = current workflow; `reference` = useful but rarely run; `superseded` = replaced by a newer script |

---

## 1. Core Pipeline Tools

The foundational export and build pipeline.

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `oxport.py` | Full XPN export pipeline orchestrator — chains render-spec → drum-export → keygroup-export → packager in sequence | `Presets/XOceanus/*.xometa`, pack config | `.xpn` zip bundle | active |
| `xpn_drum_export.py` | Export `.xometa` drum presets to MPC-compatible XPM drum programs | `.xometa` drum presets | `Output/XPM/` XPM XML files | active |
| `xpn_keygroup_export.py` | Export `.xometa` keygroup presets to MPC-compatible XPM keygroup programs | `.xometa` keygroup presets | `Output/XPM/` XPM XML files | active |
| `xpn_kit_expander.py` | Expand a single kit stub into velocity-layered / round-robin XPM programs | Kit spec JSON / XPM stub | Expanded XPM programs | active |
| `xpn_packager.py` | Package XPM programs + samples + metadata into a `.xpn` ZIP | `Output/XPM/`, sample pool | `.xpn` ZIP archive | active |
| `xpn_bundle_builder.py` | Build multi-pack bundles from multiple `.xpn` files | Individual `.xpn` packs | Bundle `.xpn` | active |
| `xpn_batch_export.py` | Batch-orchestrate many XPN exports from a manifest | `pack_manifest.json` | Multiple `.xpn` bundles | active |
| `xpn_render_spec.py` | Generate render specs (tempo, root note, loop points) from `.xometa` | `.xometa` files | `render_spec.json` | active |
| `xpn_render_spec_generator.py` | Extended render spec generator with per-engine physical defaults | `.xometa` files | `render_spec.json` | active |
| `xpn_xometa_to_xpm_exporter.py` | Convert `.xometa` preset directly to XPM XML | `.xometa` files | XPM XML | active |
| `xpn_xometa_to_pack_bridge.py` | Bridge from `.xometa` fleet to pack assembly pipeline | `.xometa` files | Pack staging dir | active |
| `xpn_xometa_export_pipeline.py` | End-to-end `.xometa` → XPN export pipeline wrapper | `.xometa` files, config | `.xpn` bundle | active |
| `xpn_free_pack_pipeline.py` | Build the free sampler pack with curated presets | `.xometa` selection | Free pack `.xpn` | active |
| `xpn_submission_packager.py` | Package a finished pack for marketplace submission | Pack dir | Submission-ready ZIP | active |
| `xpn_to_sfz.py` | Convert XPN keygroup program to SFZ format | `.xpn` / XPM files | `.sfz` file | reference |

---

## 2. Preset Generation

Scripts that write new `.xometa` preset files.

### 2a. Engine-Specific Generators

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `generate_constellation_presets.py` | Generate 530 presets for 5 Constellation Fast Track engines (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) | none | `Presets/XOceanus/` `.xometa` files | active |
| `generate_coupling_presets.py` | Generate cross-engine coupling preset stubs for newer engines | none | `.xometa` coupling presets | active |
| `generate_library_fills.py` | Fill mood/engine gaps for Overdub, Odyssey, OddOscar, ONSET, OddfeliX | none | `.xometa` gap-fill presets | active |
| `generate_onset_presets.py` | Generate ONSET (XOnset) factory presets | none | `Presets/XOceanus/` `.xometa` files | reference |
| `generate_opal_presets.py` | Generate 100 XOpal factory presets across 5 granular categories | none | `Presets/XOceanus/` `.xometa` files | reference |
| `generate_organon_presets.py` | Generate 120 XOrganon factory presets | none | `Presets/XOceanus/` `.xometa` files | reference |
| `generate_organon_coupling_presets.py` | Generate Organon + other engine coupling presets | none | `.xometa` coupling presets | reference |
| `generate_ouroboros_presets.py` | Generate XOuroboros (chaotic attractor) factory presets | none | `Presets/XOceanus/` `.xometa` files | reference |
| `generate_overbite_presets.py` | Generate XOverbite (BITE) factory presets | none | `Presets/XOceanus/` `.xometa` files | reference |
| `generate_overworld_presets.py` | Generate 40 XOverworld (chip synth) factory presets | none | `Presets/XOceanus/` `.xometa` files | reference |
| `generate_preset_params.py` | Generate DSP parameter values for `.xocmeta` stubs from metadata heuristics | `.xocmeta` stubs | Populated `.xocmeta` | superseded |

### 2b. Diversity Expansion Packs

These scripts generate presets targeting specific DNA extremes or mood gaps. Grouped by mood/target:

**Fleet-Wide / Multi-Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_final_diversity_push.py` | Fleet-wide push to fill remaining DNA extreme zones | active |
| `xpn_diversity_threshold_crosser.py` | Generate presets to push fleet diversity score past threshold | active |
| `xpn_coverage_final_sprint.py` | Final sprint to fill remaining engine/mood coverage gaps | active |
| `xpn_flagship_trio_deep_expansion.py` | Deep coupling expansion for OVERDUB, ODYSSEY, OPAL | active |
| `xpn_founding_pair_deep_expansion.py` | Deep expansion for the founding engine pair | active |
| `xpn_overlap_outwit_deep_expansion.py` | Deep DNA expansion for OVERLAP + OUTWIT | active |
| `xpn_brightness_expansion_pack.py` | Expand brightness dimension across fleet | active |
| `xpn_aggression_expansion_pack.py` | Expand aggression dimension across fleet | active |
| `xpn_movement_aggression_expander.py` | Expand both movement + aggression extremes | active |
| `xpn_space_expander.py` | Expand space dimension across fleet | active |
| `xpn_space_dimension_expander.py` | Extended space expander v2 | active |
| `xpn_warmth_cold_dense_expander.py` | Expand cold-dense zone across fleet | active |
| `xpn_dna_extreme_zone_injector.py` | Inject presets into underrepresented DNA extreme zones | active |
| `xpn_brightness_midrange_drain.py` | Remove midrange-brightness clustering by adding extremes | superseded |
| `xpn_brightness_midrange_drain_v2.py` | V2 of brightness midrange drain | active |
| `xpn_extreme_brightness_v2_pack.py` | Pack targeting extreme brightness values | active |
| `xpn_preset_dna_extreme_filler.py` | Meta-filler for all DNA extreme zones | active |
| `xpn_preset_movement_expander.py` | Expand movement dimension extremes | active |

**Foundation Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_foundation_expansion_pack.py` | 60 Foundation mood expansion presets | reference |
| `xpn_foundation_ultra_diverse.py` | Ultra-diverse Foundation presets | active |
| `xpn_foundation_prism_diversity_anchors.py` | Foundation + Prism diversity anchors | active |
| `xpn_foundation_prism_mega_diverse.py` | Mega-diverse Foundation + Prism batch | active |
| `xpn_foundation_movement_aggression_pack.py` | Foundation movement + aggression gap fill | active |
| `xpn_foundation_aether_gap_pack.py` | Foundation + Aether gap closure pack | active |

**Atmosphere Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_atmosphere_expansion_pack.py` | 60 Atmosphere mood expansion presets | reference |
| `xpn_atmosphere_ultra_diverse.py` | Ultra-diverse Atmosphere presets | active |
| `xpn_atmosphere_diversity_expander.py` | Expand Atmosphere diversity | active |
| `xpn_atmosphere_aggression_expander.py` | Atmosphere aggression extremes | active |
| `xpn_atmosphere_gap_pack.py` | Atmosphere gap closure | active |
| `xpn_atmosphere_prism_ultra_diverse.py` | 80 presets: 40 Atmosphere + 40 Prism | active |
| `xpn_atmosphere_flux_mega_diverse.py` | Mega-diverse Atmosphere + Flux batch | active |

**Entangled Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_entangled_ultra_diverse.py` | Ultra-diverse Entangled presets | active |
| `xpn_entangled_continuation_ultra_diverse.py` | Continuation ultra-diverse Entangled batch | active |
| `xpn_entangled_final_diversity_push.py` | Final diversity push for Entangled | active |
| `xpn_entangled_aggression_extremes_pack.py` | Entangled extreme aggression presets | active |
| `xpn_entangled_brutal_aggression_pack.py` | Entangled brutal aggression pack | active |
| `xpn_entangled_warmth_extremes_pack.py` | Entangled warmth extremes | active |
| `xpn_entangled_hot_warmth_pack.py` | Entangled hot/warm zone fill | active |
| `xpn_entangled_cold_sparse_pack.py` | Entangled cold-sparse zone fill | active |
| `xpn_entangled_warm_dense_pack.py` | Entangled warm-dense zone fill | active |
| `xpn_entangled_dark_brightness_pack.py` | Entangled dark-brightness zone fill | active |
| `xpn_entangled_density_high_pack.py` | Entangled high density zone | active |
| `xpn_entangled_density_low_pack.py` | Entangled low density zone | active |
| `xpn_entangled_movement_high_pack.py` | Entangled XHIGH movement zone | active |
| `xpn_entangled_movement_low_pack.py` | Entangled low movement zone | active |
| `xpn_entangled_space_extremes_pack.py` | Entangled space extremes | active |
| `xpn_entangled_high_variance_pack.py` | Entangled high-variance presets | active |
| `xpn_entangled_preset_fixer.py` | Add second-engine coupling to single-engine Entangled presets | reference |

**Prism Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_prism_expansion_pack.py` | XOceanus Prism mood expansion | reference |
| `xpn_prism_ultra_diverse.py` | Ultra-diverse Prism presets | active |
| `xpn_prism_gap_pack.py` | Prism gap closure pack | active |
| `xpn_prism_flux_bright_extreme_pack.py` | Prism + Flux bright-extreme zone fill | active |

**Flux Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_flux_dna_expansion_pack.py` | Flux DNA expansion | reference |
| `xpn_flux_diversity_expansion.py` | Flux diversity expansion | active |
| `xpn_flux_ultra_diverse.py` | Ultra-diverse Flux presets | active |
| `xpn_flux_ultra_diverse_v2.py` | Flux ultra-diverse v2 | active |
| `xpn_flux_gap_pack.py` | Flux gap closure | active |
| `xpn_flux_aggression_pack.py` | Flux aggression extremes | active |

**Aether Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_aether_expansion_pack.py` | 60 Aether mood expansion presets | reference |
| `xpn_aether_dark_cold_expansion.py` | Aether dark-cold zone fill | active |
| `xpn_aether_family_ultra_diverse.py` | 80 presets: 40 Aether + 40 Family | active |

**Family Mood**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_family_expansion_pack.py` | Family mood expansion presets | reference |
| `xpn_family_mood_expander.py` | Generate Family mood presets | active |
| `xpn_family_dna_extremes_pack.py` | Family DNA extreme zone fill | active |
| `xpn_family_extreme_anchors.py` | Family extreme anchor presets | active |
| `xpn_family_portrait_generator.py` | Family portrait preset collection | active |

**Temperature / Density Dimension Packs**

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_cold_dense_preset_pack.py` | Cold + dense zone fill | active |
| `xpn_cold_sparse_preset_pack.py` | Cold + sparse zone fill | active |
| `xpn_hot_dense_preset_pack.py` | Hot + dense zone fill | active |
| `xpn_hot_sparse_preset_pack.py` | Hot + sparse zone fill | active |

### 2c. Engine-Pair Coupling Packs

Scripts generating coupling presets for specific engine pairs. All write to `Presets/XOceanus/` as `.xometa` files.

| Script | Engine Pair(s) | Status |
|--------|---------------|--------|
| `xpn_constellation_coupling_pack.py` | OHM/ORPHICA/OBBLIGATO/OTTONI/OLE × each other | reference |
| `xpn_constellation_external_coupling_pack.py` | Constellation engines × 6 external engines | active |
| `xpn_constellation_complete_coverage_pack.py` | Complete Constellation coverage sweep | active |
| `xpn_obbligato_coupling_pack.py` | OBBLIGATO couplings | active |
| `xpn_ohm_orphica_ottoni_ole_coupling_pack.py` | OHM/ORPHICA/OTTONI/OLE couplings | active |
| `xpn_ottoni_ole_orphica_coverage_pack.py` | OTTONI/OLE/ORPHICA Entangled coverage | active |
| `xpn_oblong_onset_coupling_pack.py` | OBLONG × ONSET | active |
| `xpn_obese_overbite_coupling_pack.py` | OBESE × OVERBITE | active |
| `xpn_obese_gap_closure_pack.py` | OBESE gap closure | active |
| `xpn_overbite_gap_closure_pack.py` | OVERBITE gap closure | active |
| `xpn_overworld_oblong_coupling_pack.py` | OVERWORLD × OBLONG | active |
| `xpn_oracle_odyssey_coupling_pack.py` | ORACLE × ODYSSEY | active |
| `xpn_overdub_opal_coupling_pack.py` | OVERDUB × OPAL | active |
| `xpn_organon_ouroboros_coupling_pack.py` | ORGANON × OUROBOROS | active |
| `xpn_ouroboros_organon_deep_coupling_pack.py` | OUROBOROS × ORGANON deep expansion | active |
| `xpn_oddfelix_oddoscar_coupling_pack.py` | ODDFELIX × ODDOSCAR | active |
| `xpn_overlap_outwit_coupling_pack.py` | OVERLAP × OUTWIT | active |
| `xpn_outwit_overlap_coupling_pack.py` | OUTWIT × OVERLAP (reverse focus) | active |
| `xpn_osprey_osteria_coupling_pack.py` | OSPREY × OSTERIA | active |
| `xpn_owlfish_coupling_pack.py` | OWLFISH zero-coverage gap fill | active |
| `xpn_ombre_orca_octopus_coupling_pack.py` | OMBRE/ORCA/OCTOPUS trio | active |
| `xpn_orca_octopus_ombre_trio_pack.py` | ORCA/OCTOPUS/OMBRE trio (alternate) | active |
| `xpn_orca_osteria_owlfish_coverage_pack.py` | ORCA/OSTERIA/OWLFISH coverage | active |
| `xpn_oceanic_ocelot_coupling_pack.py` | OCEANIC × OCELOT | active |
| `xpn_ocelot_orbital_coupling_pack.py` | OCELOT × ORBITAL | active |
| `xpn_obsidian_obscura_coupling_pack.py` | OBSIDIAN × OBSCURA | active |
| `xpn_obsidian_oracle_coupling_pack.py` | OBSIDIAN × ORACLE | active |
| `xpn_obscura_ocelot_oblique_coverage_pack.py` | OBSCURA/OCELOT/OBLIQUE coverage | active |
| `xpn_obscura_origami_orbital_coupling_pack.py` | OBSCURA/ORIGAMI/ORBITAL couplings | active |
| `xpn_octopus_ombre_deep_coverage_pack.py` | OCTOPUS × OMBRE deep coverage | active |
| `xpn_odyssey_oblong_obese_coverage_pack.py` | ODYSSEY/OBLONG/OBESE coverage | active |
| `xpn_optic_oblique_ocelot_coupling_pack.py` | OPTIC/OBLIQUE/OCELOT | active |
| `xpn_origami_optic_oblique_coupling_pack.py` | ORIGAMI/OPTIC/OBLIQUE visual trio | active |
| `xpn_orbital_overbite_obese_pack.py` | ORBITAL/OVERBITE/OBESE | active |
| `xpn_rhythm_engines_coupling_pack.py` | OBLONG/ONSET/OVERWORLD rhythm trio | active |
| `xpn_shore_engines_coupling_pack.py` | Shore engines Entangled coupling | active |
| `xpn_dark_engines_coupling_pack.py` | Four dark character engines coupling | active |
| `xpn_legacy_coupling_pack.py` | Legacy Entangled coupling stubs | superseded |
| `xpn_oddfelix_oddoscar_coupling_pack.py` | ODDFELIX × ODDOSCAR | active |
| `xpn_oddfelix_oddoscar_coupling_pack.py` | OddfeliX/OddOscar couplings | active |
| `xpn_coupling_recipe_expander.py` | Expand coupling recipes into preset stubs | active |
| `xpn_coupling_coverage_tracker_v2.py` | Track coupling coverage across all engine pairs | active |
| `xpn_fleet_coupling_coverage_updater.py` | Update fleet coupling coverage snapshot | active |

### 2d. Speciality / Experimental Preset Kits

| Script | Purpose | Status |
|--------|---------|--------|
| `xpn_attractor_kit.py` | Kit using chaotic attractor dynamics | reference |
| `xpn_braille_rhythm_kit.py` | Rhythm kit based on Braille dot patterns | reference |
| `xpn_ca_presets_kit.py` | Cellular automata preset kit | reference |
| `xpn_climate_kit.py` | Climate-themed preset kit | reference |
| `xpn_entropy_kit.py` | Entropy / noise-based preset kit | reference |
| `xpn_gene_kit.py` | Genetic/biological themed kit | reference |
| `xpn_gravitational_wave_kit.py` | Gravitational wave physics preset kit | reference |
| `xpn_lsystem_kit.py` | L-System rhythm kit | reference |
| `xpn_pendulum_kit.py` | Pendulum physics preset kit | reference |
| `xpn_poetry_kit.py` | Poetry / spoken word themed kit | reference |
| `xpn_seismograph_kit.py` | Seismograph / tremor themed kit | reference |
| `xpn_transit_kit.py` | Transit / transport themed kit | reference |
| `xpn_turbulence_kit.py` | Turbulence / chaos themed kit | reference |
| `xpn_tide_tables_builder.py` | Tide Tables expansion preset builder | active |
| `xpn_tide_tables_pack_builder.py` | Tide Tables pack assembler | active |
| `xpn_tide_tables_build_validator.py` | Validate Tide Tables pack build | active |
| `xpn_ombre_orca_octopus_presets.py` | OMBRE/ORCA/OCTOPUS standalone presets | reference |

---

## 3. Analysis & Reporting

Read-only scripts that report on the fleet without writing presets.

### 3a. DNA / Diversity Analysis

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_dna_diversity_analyzer.py` | Fleet-wide 6D cosine diversity score + per-mood breakdown | `Presets/XOceanus/*.xometa` | Console report + `Docs/snapshots/dna_diversity_analysis.json` | active |
| `xpn_fleet_diversity_score_v2.py` | Fleet diversity score v2 (improved sampling) | `.xometa` fleet | Score report | active |
| `xpn_fleet_dna_diversity_report.py` | Full per-engine DNA diversity report | `.xometa` fleet | DNA report | active |
| `xpn_dna_gap_finder.py` | Find underrepresented zones in 6D DNA space | `.xometa` fleet | Gap report | active |
| `xpn_preset_clustering_report.py` | Identify clusters of near-duplicate DNA vectors | `.xometa` fleet | Cluster report | active |
| `xpn_preset_similarity_matrix.py` | Compute pairwise DNA similarity matrix | `.xometa` fleet | Similarity matrix | reference |
| `audit_sonic_dna.py` | Per-engine Sonic DNA coverage auditor | `.xometa` fleet | Console coverage table | active |
| `compute_preset_dna.py` | Compute 6D DNA for every preset (batch) | `.xometa` fleet | Updates DNA fields in `.xometa` | reference |
| `find_missing_dna.py` | List presets with missing or incomplete DNA blocks | `.xometa` fleet | Console list | active |

### 3b. Fleet Health & Coverage

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_fleet_health_dashboard.py` | Full fleet health dashboard (engine counts, mood balance, DNA) | `.xometa` fleet | Console dashboard | active |
| `xpn_fleet_health_summary.py` | Compact fleet health summary | `.xometa` fleet + `--preset-dir` | Console summary | active |
| `xpn_fleet_mood_balancer.py` | Analyze mood distribution and flag imbalances | `.xometa` fleet | Mood balance report | active |
| `xpn_engine_coverage_mapper.py` | Map preset counts per engine × mood | `.xometa` fleet | Coverage matrix | active |
| `xpn_engine_preset_gap_reporter.py` | Report engines below target preset count | `.xometa` fleet | Gap list | active |
| `xpn_per_mood_gap_report.py` | Per-mood gap report with engine breakdowns | `.xometa` fleet | Gap report (saved to `Docs/snapshots/`) | active |
| `xpn_coupling_density_heatmap.py` | Coupling density heatmap across engine pairs | `.xometa` fleet | Heatmap report | active |
| `xpn_pack_health_dashboard.py` | Pack-level health dashboard | Pack catalog | Dashboard | active |
| `xpn_expansion_bundle_profiler.py` | Profile expansion bundles for size/coverage | Bundle files | Profile report | reference |
| `validate_presets.py` | Comprehensive `.xometa` schema + DNA QA | `.xometa` fleet | Validation report | active |
| `xpn_full_qa_runner.py` | Master QA orchestrator — runs all validators in sequence | `.xometa` fleet | Full QA report | active |
| `xpn_pack_qa_ci_runner.py` | Single-command CI quality gate | `.xometa` fleet | Pass/fail + report | active |
| `xpn_pack_qa_report.py` | Comprehensive pack QA report | Pack dir | QA report | active |

### 3c. Pack Analytics

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_pack_analytics.py` | Fleet catalog analytics (counts, tags, moods) | `.xometa` fleet | Analytics report | active |
| `xpn_pack_compare_report.py` | Side-by-side comparison of two packs | Two pack dirs | Diff report | reference |
| `xpn_pack_diff.py` | Diff two pack versions | Two `.xpn` files | Diff output | reference |
| `xpn_pack_bundle_sizer.py` | Estimate `.xpn` bundle size before building | Pack staging dir | Size estimate | active |
| `xpn_pack_diet_analyzer.py` | Analyze pack "diet" — what's included vs redundant | Pack dir | Diet report | reference |
| `xpn_pack_index_generator.py` | Generate browsable pack index | Pack dir | `pack_index.json` | active |
| `xpn_pack_catalog_generator.py` | Generate full pack catalog document | Pack registry | Catalog doc | active |
| `xpn_pack_registry.py` | Manage/query the XPN pack registry | Registry JSON | Registry operations | active |
| `xpn_preset_fleet_snapshot.py` | Take a point-in-time fleet snapshot | `.xometa` fleet | `Docs/snapshots/fleet_*.json` | active |
| `xpn_session_summary_generator.py` | Generate session summary document | Tool logs / fleet state | Session summary | reference |
| `xpn_session_handoff.py` | Generate session handoff briefing | Fleet state | Handoff doc | reference |

---

## 4. Validation & Quality Assurance

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_validator.py` | XPN/XPM pack validator + linter | `.xpn` / XPM files | Validation report | active |
| `xpn_xpm_validator_strict.py` | Strict MPC XPM XML schema validator | XPM XML files | Strict validation report | active |
| `xpn_pack_format_validator.py` | Deep XPN/XPM format validator | Pack dir | Format report | active |
| `xpn_pack_integrity_check.py` | Check pack file integrity (hashes, completeness) | `.xpn` pack | Integrity report | active |
| `xpn_pack_naming_validator.py` | Validate XO_OX brand naming conventions | Pack files | Naming report | active |
| `xpn_manifest_validator.py` | Validate `manifest.json` / `expansion.json` | Manifest files | Validation report | active |
| `xpn_manifest_generator.py` | Generate `manifest.json` for a pack | Pack dir | `manifest.json` | active |
| `xpn_expansion_json_builder.py` | Build and validate `expansion.json` | Pack dir | `expansion.json` | active |
| `xpn_expansion_json_lint.py` | Lint `expansion.json` | `expansion.json` | Lint report | active |
| `xpn_kit_validator.py` | Validate kit structure and completeness | Kit dir | Validation report | active |
| `xpn_kit_completeness.py` | Score drum kit slot completeness | Kit / XPM | Completeness score | active |
| `xpn_xometa_batch_validator.py` | Batch validate all `.xometa` files for schema | `.xometa` fleet | Batch validation report | active |
| `xpn_xometa_completeness_auditor.py` | Audit `.xometa` field completeness | `.xometa` fleet | Completeness report | active |
| `xpn_macro_coverage_checker.py` | Check M1-M4 macro assignment coverage | `.xometa` fleet | Coverage report | active |
| `xpn_tuning_coverage_checker.py` | Check tuning system coverage across fleet | `.xometa` fleet | Tuning report | active |
| `xpn_community_qa.py` | QA orchestrator for community pack submissions | Community pack dir | QA report | active |
| `xpn_stems_checker.py` | MPCe quad-corner pack readiness validator | Pack dir | Readiness report | active |
| `xpn_layer_balance_checker.py` | Check velocity layer balance in programs | XPM / pack | Balance report | active |

---

## 5. Repair & Migration

Scripts that modify existing presets to fix issues or migrate formats.

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_clone_cluster_repair.py` | Find and fix clone clusters (near-identical DNA vectors) | `.xometa` fleet | Patched `.xometa` files | active |
| `xpn_preset_dna_fixer.py` | Auto-repair incomplete or invalid DNA blocks | `.xometa` fleet | Repaired `.xometa` files | active |
| `xpn_preset_dna_recalibrator.py` | Recalibrate DNA values to match updated engine profiles | `.xometa` fleet | Recalibrated `.xometa` files | active |
| `xpn_preset_name_dedup_fixer.py` | Fix preset name collisions across the fleet | `.xometa` fleet | Renamed `.xometa` files | active |
| `xpn_preset_tag_normalizer.py` | Standardize tags across `.xometa` fleet | `.xometa` fleet | Normalized `.xometa` files | active |
| `xpn_preset_mood_rebalancer.py` | Rebalance mood distribution by patching under-represented presets | `.xometa` fleet | Patched `.xometa` files | active |
| `xpn_preset_rebalancer.py` | General preset collection rebalancer | `.xometa` fleet | Rebalanced `.xometa` | active |
| `xpn_macro_label_normalizer.py` | Normalize macro label text across fleet | `.xometa` fleet | Patched `.xometa` files | active |
| `xpn_entangled_preset_fixer.py` | Add missing second-engine coupling to single-engine Entangled presets | `.xometa` Entangled presets | Patched presets | reference |
| `fix_bob_presets.py` | Migrate XOblong presets from standalone → XOceanus param schema | `Presets/XOceanus/` Bob presets | Migrated `.xometa` | reference |
| `fix_drift_presets.py` | Migrate XOdyssey presets from standalone → XOceanus param schema | `Presets/XOceanus/` Drift presets | Migrated `.xometa` | reference |
| `fix_dub_presets.py` | Migrate XOverdub presets from standalone → XOceanus param schema | `Presets/XOceanus/` Dub presets | Migrated `.xometa` | reference |
| `fix_overworld_presets.py` | Fix Overworld presets using wrong UPPER_SNAKE_CASE param keys | `Presets/XOceanus/` Overworld presets | Fixed `.xometa` | reference |
| `fix_xobese_xpms.py` | Patch broken XObese keygroup XPM programs for MPC compliance | XObese XPM files | Patched XPM files | reference |
| `migrate_xocmeta_to_xometa.py` | Convert OddfeliX `.xocmeta` → unified `.xometa` format | `.xocmeta` files | `.xometa` files | superseded |
| `add_missing_dna.py` | Auto-populate missing Sonic DNA blocks via heuristics | `.xometa` files without DNA | Updated `.xometa` files | active |
| `xpn_xometa_schema_version_migrator.py` | Migrate `.xometa` files across schema versions | Old-schema `.xometa` | Migrated `.xometa` | reference |
| `rename_weak_presets.py` | Elevate generic preset names to evocative XO_OX-brand names | `.xometa` files | Renamed `.xometa` files | reference |
| `apply_renames.py` | Bulk rename engine references (e.g. XOddCouple → OddfeliX) | `.xometa` / docs / tools | Renamed files | superseded |
| `xpn_program_renamer.py` | Batch-rename XPM programs inside a `.xpn` archive | `.xpn` ZIP | Updated `.xpn` | active |

---

## 6. DNA / Metadata Computation

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_auto_dna.py` | Compute 6D Sonic DNA from WAV audio (spectral analysis) | WAV files | DNA JSON / `.xometa` DNA field | active |
| `xpn_engine_dna_profile_builder.py` | Build per-engine DNA profile from fleet statistics | `.xometa` fleet | Engine DNA profiles JSON | active |
| `xpn_dna_interpolator.py` | Interpolate DNA values between two preset endpoints | Two `.xometa` files | Interpolated DNA chain | active |
| `xpn_sonic_dna_interpolator_v2.py` | DNA interpolator v2 with improved curve shaping | Two `.xometa` files | Interpolated DNA chain | active |
| `xpn_preset_batch_generator.py` | Batch generate `.xometa` stubs for a given engine | Engine spec | `.xometa` stub batch | active |
| `xpn_preset_name_generator.py` | Generate evocative preset names from Sonic DNA vector | DNA vector | Name suggestions | active |
| `xpn_preset_variation_generator.py` | Generate variations of an existing preset | Source `.xometa` | Variation `.xometa` files | active |
| `xpn_variation_generator.py` | XPM preset variation generator | XPM file | Variation XPM files | active |
| `xpn_auto_root_detect.py` | Auto-detect fundamental pitch of a WAV sample | WAV file | Root note (MIDI #) | active |
| `xpn_classify_instrument.py` | Rule-based instrument type classifier for WAV audio | WAV file | Instrument class label | active |
| `xpn_program_type_classifier.py` | XPN program type classifier (drum vs keygroup) | XPM / pack | Program type | active |
| `breed_presets.py` | Breed two parent presets via DNA crossover + mutation | Two `.xometa` files | Offspring `.xometa` | reference |
| `xpn_monster_rancher.py` | Generate novel presets from multi-parent DNA mixing | Multiple `.xometa` | Hybrid `.xometa` | reference |
| `xpn_engine_intro_preset_builder.py` | Build intro/demo preset for a new engine | Engine spec | Intro `.xometa` | reference |
| `xpn_xometa_dna_bulk_export.py` | Bulk-export DNA fields from all `.xometa` to CSV/JSON | `.xometa` fleet | `dna_bulk_export.csv/json` | active |

---

## 7. Velocity & Pad Tools

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_adaptive_velocity.py` | Auto-shape velocity curves in XPM programs based on sample transients | XPM XML files | Updated XPM XML | active |
| `xpn_velocity_curve_designer.py` | Design custom velocity response curves | User-specified curve params | Velocity curve JSON / XPM | active |
| `xpn_velocity_curve_tester.py` | Test and visualize velocity curves against sample data | XPM / velocity curve | Test report | reference |
| `xpn_velocity_map_visualizer.py` | Visualize velocity zone map for a program | XPM file | ASCII velocity map | reference |
| `xpn_velocity_zone_visualizer.py` | Show velocity zone boundaries per layer | XPM file | Zone visualization | reference |
| `xpn_pad_note_mapper.py` | Map MIDI notes to MPC pad layout | MIDI map spec | Pad note mapping | active |
| `xpn_pad_layout_visualizer.py` | Visualize 16-pad layout with assignments | XPM file | ASCII pad grid | reference |
| `xpn_pad_label_optimizer.py` | Optimize pad label strings for MPC screen | XPM file | Updated XPM | active |
| `xpn_choke_group_assigner.py` | Assign choke/mute groups to drum programs | XPM drum file | Updated XPM | active |
| `xpn_choke_group_designer.py` | Design choke group relationships for a kit | Kit spec | Choke group config | reference |
| `xpn_mpce_quad_builder.py` | Build MPCe quad-corner (4-pad) performance layout | Preset spec | Quad XPM | active |
| `xpn_articulation_builder.py` | Build multi-articulation XPM program | Articulation spec | XPM with articulations | active |

---

## 8. Sample Tools

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_sample_categorizer.py` | Categorize WAV samples by instrument type and character | WAV directory | Categorized sample manifest | active |
| `xpn_sample_audit.py` | Audit WAV samples in a pack before release | WAV / `.xpn` directory | Audit report | active |
| `xpn_sample_rate_auditor.py` | Check sample rates across a pack (flag non-44.1kHz/48kHz) | WAV files | Sample rate report | active |
| `xpn_sample_fingerprinter.py` | SHA fingerprint WAV samples for deduplication + provenance | WAV directory | Fingerprint DB | active |
| `xpn_dedup_checker.py` | Check for duplicate WAV samples by SHA-256 hash | WAV directory | Dedup report | active |
| `xpn_sample_loop_checker.py` | Check WAV loop point quality | WAV / `.xpn` | Loop quality report | active |
| `xpn_sample_loop_optimizer.py` | Optimize WAV loop points to zero-crossings | WAV files | Patched WAV files | active |
| `xpn_smart_trim.py` | Auto-trim silence from WAV samples + detect loop points | WAV files | Trimmed WAV files | active |
| `xpn_normalize.py` | Normalize WAV sample levels | WAV files | Normalized WAV files | active |
| `xpn_sample_rename_batch.py` | Batch rename WAV samples per naming convention | WAV directory | Renamed WAV files | active |
| `xpn_sample_metadata_embedder.py` | Embed metadata tags into WAV files (BEXT / iXML) | WAV files + metadata | Tagged WAV files | reference |
| `xpn_sample_pool_builder.py` | Build a curated sample pool from multiple sources | Source WAV dirs | Pooled sample directory | active |
| `xpn_sample_category_report.py` | Report on sample categories in a pack | Sample pool | Category report | reference |

---

## 9. Cover Art & Visual Tools

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_cover_art.py` | Generate pack cover art (SVG/PNG) from engine metadata | Engine metadata | `cover_art.svg/png` | active |
| `xpn_cover_art_generator_v2.py` | Cover art generator v2 with improved layouts | Engine metadata | `cover_art.svg/png` | active |
| `xpn_cover_art_batch.py` | Batch generate cover art for all packs | Pack registry | Cover art files per pack | active |
| `xpn_cover_art_audit.py` | Audit cover art files for presence and spec compliance | Pack dirs | Audit report | active |
| `xpn_pack_thumbnail_generator.py` | Generate pack thumbnail images | Pack metadata | Thumbnail PNG | active |
| `xpn_pack_thumbnail_spec.py` | Generate thumbnail design brief document | Pack metadata | Design brief | reference |
| `xpn_optic_fingerprint.py` | Generate visual "fingerprint" image from DNA vector | DNA vector | Fingerprint image | reference |
| `xpn_quick_preview_generator.py` | Generate quick preview card for a pack | Pack metadata | Preview card image | reference |
| `xpn_liner_notes.py` | Generate liner notes document for a pack | Pack metadata | Liner notes text | reference |
| `xpn_preview_generator.py` | Generate preview audio/image assets for a pack | Pack dir | Preview assets | reference |

---

## 10. Release & Distribution Tools

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_changelog_generator.py` | Generate CHANGELOG.md from git commit history + pack diffs | Git log | `CHANGELOG.md` | active |
| `xpn_release_notes_generator.py` | Generate human-readable release notes | Pack diff / changelog | `release_notes.txt` | active |
| `xpn_pack_release_checklist.py` | Generate release checklist for a pack | Pack dir | Checklist document | active |
| `xpn_pack_version_bumper.py` | Bump version field in pack manifest | `manifest.json` | Updated manifest | active |
| `xpn_pack_ci_config_generator.py` | Generate CI config (GitHub Actions / shell) for pack pipeline | Pack spec | CI config YAML | reference |
| `xpn_pack_license_generator.py` | Generate `LICENSE.txt` for a pack | Pack metadata | `LICENSE.txt` | active |
| `xpn_pack_marketing_copy.py` | Generate marketing copy blurbs for a pack | Pack metadata | Marketing text | reference |
| `xpn_pack_revenue_estimator.py` | Estimate revenue projection for a pack | Sales model params | Revenue estimate | reference |
| `xpn_pack_localization_guide.py` | Check pack text for localization readiness | Pack text files | Localization report | reference |
| `xpn_pack_discovery_optimizer.py` | Optimize pack metadata for discoverability | Pack metadata | Optimized metadata | reference |
| `xpn_pack_story_arc.py` | Validate narrative arc across a multi-pack series | Pack series | Arc validation | reference |
| `xpn_pack_series_planner.py` | Plan a multi-pack release series | Series spec | Series plan | reference |
| `xpn_pack_ab_tester.py` | A/B test two pack variants | Two pack dirs | Test report | reference |
| `xpn_pack_size_optimizer.py` | Optimize pack file size for distribution | Pack dir | Size optimization report | reference |
| `xpn_pack_readme_generator.py` | Generate README for a pack | Pack metadata | `README.txt/md` | active |
| `xpn_pack_readme_updater.py` | Auto-update existing pack README sections | Existing README + pack state | Updated README | active |
| `xpn_export_report_generator.py` | Generate export summary report | Export log | Report document | active |
| `xpn_oxport_pack_yml_template.py` | Generate `pack.yml` config template for oxport v2 pipeline | Pack spec | `pack.yml` | active |

---

## 11. Advanced / Experimental Tools

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `xpn_curiosity_engine.py` | Generative curiosity engine — explores DNA space randomly | Fleet DNA state | Novel preset suggestions | reference |
| `xpn_deconstruction_builder.py` | Deconstruct a preset into stems/layers | `.xometa` + samples | Stem pack | reference |
| `xpn_evolution_builder.py` | Evolve a preset over time using genetic algorithms | Source `.xometa` | Evolution chain | reference |
| `xpn_preset_aging_simulator.py` | Simulate how a preset would "age" sonically over time | `.xometa` file | Aged variant | reference |
| `xpn_preset_emotion_wheel.py` | Map presets onto a 2D emotion wheel | `.xometa` fleet | Emotion wheel plot | reference |
| `xpn_preset_evolution_tracker.py` | Track preset evolution across library iterations | Fleet history | Evolution graph | reference |
| `xpn_preset_crossfade_designer.py` | Design crossfade transitions between two presets | Two `.xometa` | Crossfade chain | reference |
| `xpn_collection_arc_validator.py` | Validate narrative/sonic arc across a collection | Collection `.xometa` | Arc report | reference |
| `xpn_collection_sequencer.py` | Sequence a collection for optimal release order | Collection metadata | Sequenced release plan | reference |
| `xpn_complement_renderer.py` | Render complement chain (engine + coupling complement) | `.xometa` + coupling | Complement audio spec | reference |
| `xpn_mood_arc_designer.py` | Design mood arc for a preset pack | Mood spec | Arc document | reference |
| `xpn_setlist_builder.py` | Build performance setlist from preset fleet | `.xometa` fleet | Setlist | reference |
| `xpn_performance_map_builder.py` | Build MPC performance map (macro assignments + layout) | Preset spec | Performance map | reference |
| `xpn_preset_recipe_book.py` | Generate preset recipe book for sound designers | `.xometa` fleet | Recipe book document | reference |
| `xpn_tutorial_builder.py` | Generate tutorial content for a pack | Pack metadata | Tutorial document | reference |
| `xpn_macro_performance_recorder.py` | Record macro movement sequences for automation | Live performance data | Macro sequence | reference |
| `xpn_macro_curve_designer.py` | Design non-linear macro response curves | Curve spec | Macro curve JSON | active |
| `xpn_macro_assignment_suggester.py` | Suggest M1-M4 macro assignments for a preset | `.xometa` file | Macro suggestions | active |
| `xpn_params_sidecar_spec.py` | Generate and validate `params_sidecar.json` | Engine param spec | `params_sidecar.json` | active |
| `xpn_tuning_systems.py` | Microtonal / alternative tuning system library | Tuning spec | Tuning table JSON | reference |
| `xpn_kit_builder_assistant.py` | Interactive kit assembly assistant | Kit spec | Kit assembly plan | reference |
| `xpn_kit_layering_advisor.py` | Advise on velocity layer structure for a kit | Kit + samples | Layer advice report | reference |
| `xpn_kit_theme_analyzer.py` | Analyze sonic theme coherence across a kit | Kit XPM | Theme analysis | reference |
| `xpn_preset_browser.py` | Terminal browser for `.xometa` presets | `.xometa` fleet | Interactive browser | reference |
| `xpn_preset_search_engine.py` | Full-text search across `.xometa` fleet | `.xometa` fleet + query | Search results | active |
| `xpn_preset_variety_scorer.py` | Score variety/diversity of a preset selection | `.xometa` selection | Variety score | active |
| `xpn_preset_duplicate_detector.py` | Detect copy-paste preset clones | `.xometa` fleet | Duplicate report | active |
| `xpn_preset_dedup_finder.py` | Find near-duplicate presets via DNA distance | `.xometa` fleet | Near-dup report | active |
| `xpn_preset_tag_recommender.py` | Recommend tags for a preset from DNA + name | `.xometa` file | Tag suggestions | active |
| `xpn_preset_mood_classifier.py` | Classify preset mood from DNA vector + tags | `.xometa` file | Mood classification | active |
| `xpn_preset_name_generator.py` | Generate evocative preset names from DNA | DNA vector | Name list | active |
| `xpn_coupling_pair_recommender.py` | Recommend engine pairs for coupling based on DNA affinity | Engine DNA profiles | Pair recommendations | active |
| `xpn_coupling_docs_generator.py` | Generate coupling documentation for all engine pairs | Fleet + coupling coverage | Coupling docs | reference |
| `xpn_coupling_recipes.py` | Recipe cards for all engine coupling archetypes | Engine specs | Coupling recipe cards | reference |
| `xpn_engine_tag_cloud_builder.py` | Build tag cloud per engine from fleet | `.xometa` fleet | Tag cloud data | reference |

---

## 12. Extraction & Migration Utilities

| Script | Purpose | Input | Output | Status |
|--------|---------|-------|--------|--------|
| `extract_cpp_presets.py` | Extract factory presets from JUCE C++ source files to `.xometa` | C++ source files (XOverdub, XOdyssey, XOblong) | `.xometa` preset files | reference |
| `xpn_xpm_batch_builder.py` | Batch build XPM programs from a preset list | Preset list + sample pool | XPM XML files | active |
| `xpn_xpm_diff_tool.py` | Diff two XPM files for regression testing | Two XPM XML files | Diff report | reference |
| `xpn_xpm_template_library.py` | Library of canonical XPM templates per program type | Template spec | XPM template files | reference |
| `xpn_bulk_metadata_editor.py` | Batch-edit metadata fields across `.xometa` files | `.xometa` fleet + edit spec | Updated `.xometa` files | active |
| `xpn_pack_changelog_tracker.py` | Track changelog entries across pack versions | Pack version history | Changelog JSON | active |
| `xpn_git_kit.py` | Git-aware kit that includes commit SHA in pack metadata | Pack + git state | Pack with git metadata | reference |
| `site_audio_export.py` | Process WAV recordings into web-ready MP3/OGG for XO-OX.org | Raw WAV recordings | `public/audio/` MP3 + OGG | active |

---

## Quick Reference — Most-Used Scripts

| Task | Script |
|------|--------|
| Export to XPN | `oxport.py` |
| Validate all presets | `validate_presets.py` or `xpn_full_qa_runner.py` |
| Check fleet diversity | `xpn_dna_diversity_analyzer.py` |
| Fleet health overview | `xpn_fleet_health_dashboard.py` |
| Find DNA gaps | `xpn_dna_gap_finder.py` |
| Fix missing DNA | `add_missing_dna.py` |
| Repair clone clusters | `xpn_clone_cluster_repair.py` |
| Take fleet snapshot | `xpn_preset_fleet_snapshot.py` |
| Generate cover art | `xpn_cover_art_generator_v2.py` |
| Build drum program | `xpn_drum_export.py` |
| Build keygroup program | `xpn_keygroup_export.py` |
| Per-mood gap report | `xpn_per_mood_gap_report.py` |
