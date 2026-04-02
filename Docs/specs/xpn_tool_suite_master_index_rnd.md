# XPN Tool Suite — Master Index
**XO_OX Designs | Tools/ Directory**
*Last updated: 2026-03-16*

---

## Overview

The `Tools/` directory contains **250+ Python scripts** covering the full XPN production lifecycle — from engine preset generation through DNA analysis, pack building, QA validation, and release publishing. Scripts divide into six categories plus a legacy/utilities group.

---

## 1. Coupling Pack Generators

Scripts that generate `.xometa` Entangled coupling preset stubs for engine pairs. Each file seeds coupling heatmap coverage for specific engine groupings.

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_constellation_coupling_pack.py` | 10 intra-Constellation pairs + 5 legacy bridges for OHM/ORPHICA/OBBLIGATO/OTTONI/OLE | `.xometa` coupling preset stubs |
| `xpn_legacy_coupling_pack.py` | Seed coverage for high-affinity legacy pairs with zero Entangled presets | `.xometa` coupling preset stubs |
| `xpn_obbligato_coupling_pack.py` | Fill OBBLIGATO (obbl_) zero-coverage gap across partner engines | `.xometa` coupling preset stubs |
| `xpn_obese_overbite_coupling_pack.py` | OBESE (fat_) × OVERBITE (poss_) marquee pair + partners | `.xometa` coupling preset stubs |
| `xpn_oblong_onset_coupling_pack.py` | OBLONG (bob_) × ONSET (perc_) rhythm-backbone pair + partners | `.xometa` coupling preset stubs |
| `xpn_obsidian_obscura_coupling_pack.py` | OBSIDIAN × OBSCURA cold/crystalline pair + partners | `.xometa` coupling preset stubs |
| `xpn_obsidian_oracle_coupling_pack.py` | OBSIDIAN × ORACLE (crystal meets prophecy) + partners | `.xometa` coupling preset stubs |
| `xpn_oceanic_ocelot_coupling_pack.py` | OCEANIC (teal) × OCELOT (tawny) biome pair + partners | `.xometa` coupling preset stubs |
| `xpn_ocelot_orbital_coupling_pack.py` | Seed coupling coverage for OCELOT and ORBITAL | `.xometa` coupling preset stubs |
| `xpn_oddfelix_oddoscar_coupling_pack.py` | ODDFELIX (snap_) × ODDOSCAR (morph_) founding sibling pair + partners | `.xometa` coupling preset stubs |
| `xpn_ohm_orphica_ottoni_ole_coupling_pack.py` | 6 intra-Constellation pairs for the 4 smaller Constellation engines + external bridges | `.xometa` coupling preset stubs |
| `xpn_ombre_orca_octopus_coupling_pack.py` | OMBRE × ORCA × OCTOPUS three-way deep-ocean coverage | `.xometa` coupling preset stubs |
| `xpn_oracle_odyssey_coupling_pack.py` | ORACLE × ODYSSEY marquee pair + partners | `.xometa` coupling preset stubs |
| `xpn_organon_ouroboros_coupling_pack.py` | ORGANON × OUROBOROS marquee pair + partners | `.xometa` coupling preset stubs |
| `xpn_origami_optic_oblique_coupling_pack.py` | ORIGAMI × OPTIC × OBLIQUE visual-coded engine trio | `.xometa` coupling preset stubs |
| `xpn_osprey_osteria_coupling_pack.py` | OSPREY × OSTERIA sister-shore pair + partners (ShoreSystem B012) | `.xometa` coupling preset stubs |
| `xpn_outwit_overlap_coupling_pack.py` | OUTWIT (owit_) × OVERLAP (olap_) newly installed engine pair | `.xometa` coupling preset stubs |
| `xpn_overdub_opal_coupling_pack.py` | OVERDUB (dub_) × OPAL texture-engine pair + partners | `.xometa` coupling preset stubs |
| `xpn_overlap_outwit_coupling_pack.py` | Complementary coverage pass for OVERLAP and OUTWIT | `.xometa` coupling preset stubs |
| `xpn_overworld_oblong_coupling_pack.py` | OVERWORLD × OBLONG chip/rhythm pair + partners | `.xometa` coupling preset stubs |
| `xpn_owlfish_coupling_pack.py` | Fill OWLFISH (owl_) zero-coverage gap — 12 partners, 36 presets | `.xometa` coupling preset stubs |

---

## 2. DNA / Preset Expanders & Pack Generators

Scripts that generate or expand preset collections — quadrant fills, DNA extremes, mood balance, and single-engine preset stubs.

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_aggression_expansion_pack.py` | High-aggression preset stubs for engines with low aggression scores | `.xometa` preset stubs |
| `xpn_brightness_expansion_pack.py` | Target brightness-compressed engines with extreme brightness presets | `.xometa` preset stubs |
| `xpn_cold_dense_preset_pack.py` | Cold Dense DNA quadrant presets (warmth low, density high) | `.xometa` preset stubs |
| `xpn_cold_sparse_preset_pack.py` | Cold Sparse DNA quadrant presets (warmth low, density low) | `.xometa` preset stubs |
| `xpn_extreme_brightness_v2_pack.py` | 60 extreme-brightness presets across the brightest engines | `.xometa` preset stubs |
| `xpn_hot_dense_preset_pack.py` | Hot Dense DNA quadrant presets (warmth high, density high) | `.xometa` preset stubs |
| `xpn_hot_sparse_preset_pack.py` | Hot Sparse DNA quadrant presets (warmth high, density low) | `.xometa` preset stubs |
| `xpn_coupling_recipe_expander.py` | Expand an existing coupling preset or recipe JSON into variant stubs | `.xometa` coupling variants |
| `xpn_family_mood_expander.py` | Generate Family mood multi-engine portrait presets | `.xometa` Family-mood presets |
| `xpn_kit_expander.py` | Expand a flat drum kit WAV set into velocity layers / round-robin variants | Expanded XPM kit structure |
| `xpn_preset_movement_expander.py` | Generate high-movement presets for kinetic engines | `.xometa` kinetic preset stubs |
| `xpn_space_expander.py` | Fill both extremes of the space DNA dimension across the fleet | `.xometa` preset stubs |
| `xpn_ombre_orca_octopus_presets.py` | Generate single-engine preset stubs for OMBRE, ORCA, and OCTOPUS | `.xometa` single-engine presets |
| `xpn_family_portrait_generator.py` | Generate Family Portrait presets grouping 3–5 engines in a unified sound | `.xometa` Family Portrait presets |
| `xpn_preset_batch_generator.py` | Systematically generate presets covering DNA space using grid/random/edge strategies | `.xometa` preset stubs |
| `xpn_preset_dna_extreme_filler.py` | Identify and fill under-populated extreme zones in all 6 DNA dimensions | `.xometa` preset stubs |
| `xpn_preset_rebalancer.py` | Analyze DNA centroid and suggest/generate rebalancing presets | Rebalance report + optional stubs |
| `xpn_preset_variation_generator.py` | Generate N variations from a single XPM preset via parameter sweep | XPM variation files |
| `xpn_variation_generator.py` | Generate tonal/spatial/character variations from a full XPN pack | Variant `.xpn` packs |
| `xpn_engine_intro_preset_builder.py` | Generate placeholder intro presets for V1 concept engines with no code yet | `.xometa` intro presets |

---

## 3. Analysis & Auditing Tools

Scripts that inspect, score, visualize, or report on presets, DNA, packs, or samples without modifying production assets.

### DNA & Preset Analysis

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_auto_dna.py` | Compute 6D Sonic DNA from a WAV file using stdlib only | DNA vector (JSON/stdout) |
| `xpn_dna_gap_finder.py` | Find underrepresented regions in 6D DNA space across a preset collection | Gap report (Markdown) |
| `xpn_dna_interpolator.py` | Interpolate between two `.xometa` presets and generate stubs along the path | Interpolated `.xometa` stubs |
| `xpn_sonic_dna_interpolator_v2.py` | v2 interpolator that generates new `.xometa` files at midpoints and beyond | Interpolated `.xometa` files |
| `xpn_preset_dna_recalibrator.py` | Audit declared DNA values against parameter values and flag discrepancies | Calibration diff report |
| `xpn_fleet_dna_diversity_report.py` | Deep 6D DNA diversity analysis of the full XOceanus preset fleet | Fleet DNA report (Markdown) |
| `xpn_engine_dna_profile_builder.py` | Compute authoritative per-engine DNA centroid from all its presets | Engine DNA profile JSON |
| `xpn_preset_similarity_matrix.py` | Compute pairwise cosine similarity, cluster density, and ASCII heatmap | Similarity matrix report |
| `xpn_preset_clustering_report.py` | k-means cluster presets by 6D DNA (stdlib only) | Cluster report (Markdown) |
| `xpn_preset_emotion_wheel.py` | Map presets onto a Plutchik emotion wheel using 6D DNA | Emotion wheel report |
| `xpn_preset_variety_scorer.py` | Score how much variety a preset collection has across 5 dimensions | Variety score report |
| `xpn_preset_aging_simulator.py` | Simulate how fleet DNA centroid shifts as presets are added chronologically | DNA aging timeline report |
| `xpn_preset_evolution_tracker.py` | Track preset fleet evolution over git history | Git-backed evolution report |
| `xpn_preset_fleet_snapshot.py` | Generate a point-in-time fleet snapshot JSON and human-readable summary | Fleet snapshot JSON + report |
| `xpn_preset_crossfade_designer.py` | Design crossfade chains where consecutive preset pairs can be smoothly blended | Crossfade chain spec |
| `xpn_optic_fingerprint.py` | Offline spectral analysis / fingerprint for XPN expansion packs | Spectral fingerprint JSON |

### Mood & Coverage Analysis

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_fleet_mood_balancer.py` | Analyze mood distribution across the fleet and recommend balance additions | Mood balance report |
| `xpn_preset_mood_classifier.py` | Auto-classify presets with missing or Unknown mood assignments | Updated `.xometa` mood fields |
| `xpn_preset_mood_rebalancer.py` | Analyze and suggest/apply mood rebalancing for a single engine | Mood rebalance suggestions |
| `xpn_engine_coverage_mapper.py` | Map which engines have preset coverage and highlight gaps | Engine coverage map (Markdown) |
| `xpn_engine_preset_gap_reporter.py` | Gap report: engines × moods with under-target preset counts | Gap report (Markdown) |
| `xpn_fleet_health_summary.py` | Single-pass fleet health dashboard: count, DNA, mood, coupling summary | Health dashboard (stdout/MD) |

### Coupling Analysis

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_coupling_coverage_tracker_v2.py` | Scan all Entangled presets and report coupling pair coverage across 34 engines | Coverage matrix report |
| `xpn_coupling_density_heatmap.py` | Visualize coupling pair coverage as an ASCII heatmap | ASCII heatmap (stdout) |
| `xpn_coupling_pair_recommender.py` | Recommend best engine pairs for new coupling presets by DNA distance + gap | Pair recommendation list |
| `xpn_coupling_docs_generator.py` | Extract coupling routes from a `.xometa` and generate producer documentation | Coupling docs (Markdown) |
| `xpn_coupling_recipes.py` | Bundle coupling metadata into XPN packs as producer reference cards | Coupling recipe cards |
| `xpn_fleet_coupling_coverage_updater.py` | Update and report coupling coverage after each generation wave | Updated coverage report |

### Pack & Sample Auditing

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_coupling_preset_auditor.py` | Audit coupling preset quality: route integrity, DNA consistency, completeness | Audit report |
| `xpn_cover_art_audit.py` | Audit cover art files inside a `.xpn` for XO_OX visual standards + MPC display | Cover art audit report |
| `xpn_sample_audit.py` | Audit WAV samples in a `.xpn` pack before release | Sample audit report |
| `xpn_sample_rate_auditor.py` | Scan WAV files and report sample rate / bit depth consistency | Sample rate report |
| `xpn_xometa_completeness_auditor.py` | Deep completeness audit of `.xometa` files for missing fields | Completeness report |
| `xpn_pack_compare_report.py` | Side-by-side comparison report for two pack directories | Comparison report (Markdown) |
| `xpn_pack_qa_report.py` | Orchestrate all key validation checks and produce a unified QA report | QA report (Markdown) |
| `xpn_pack_analytics.py` | Fleet catalog analytics: mood distribution, engine coverage, DNA spread | Analytics report |
| `xpn_pack_health_dashboard.py` | Pack catalog health dashboard across all released `.xpn` packs | Health dashboard report |
| `xpn_pack_score.py` | Composite quality score for an XPN pack across multiple dimensions | Pack score (0–100) |
| `xpn_pack_diet_analyzer.py` | Identify size reduction opportunities in a pack (redundant samples, compression) | Diet analysis report |
| `xpn_pack_diff.py` | Diff two versions of an `.xpn` pack for changelogs and QA | Pack diff report |
| `xpn_pack_ab_tester.py` | Compare two pack directories across 6 quality dimensions to determine release winner | A/B comparison report |
| `xpn_pack_discovery_optimizer.py` | Audit pack metadata for MPC marketplace discoverability improvements | Discoverability suggestions |
| `xpn_sample_category_report.py` | Generate a categorized inventory of all WAV samples in a `.xpn` pack | Sample category report |
| `xpn_sample_similarity_matrix.py` | (see preset similarity) | — |
| `xpn_dedup_checker.py` | SHA-256 hash index for sample deduplication across packs | Duplicate hash report |
| `xpn_preset_dedup_finder.py` | Find exact structural duplicates using three independent signals | Duplicate preset list |
| `xpn_preset_duplicate_detector.py` | Detect copy-paste presets (exact field matches) across the library | Duplicate detection report |
| `xpn_layer_balance_checker.py` | Check volume balance across velocity layers within a program | Layer balance report |
| `xpn_macro_coverage_checker.py` | Audit macro assignment coverage in `.xometa` preset files | Macro coverage report |
| `xpn_sample_loop_checker.py` | Check WAV loop point quality in a `.xpn` pack | Loop quality report |
| `xpn_stems_checker.py` | Validate stem WAV files for MPC expansion pack readiness (MPCe Quad) | Stems readiness report |
| `xpn_tuning_coverage_checker.py` | Detect accidental tuning inconsistencies across programs in a pack | Tuning inconsistency report |
| `xpn_kit_completeness.py` | Check completeness of a drum kit spec against required voices | Completeness checklist |
| `xpn_expansion_bundle_profiler.py` | Profile an expansion bundle: program counts, sample totals, DNA spread | Bundle profile report |
| `xpn_kit_theme_analyzer.py` | Analyze the sonic theme consistency of a drum kit | Theme analysis report |
| `xpn_kit_layering_advisor.py` | Advise on optimal layering strategy for a drum kit based on voice types | Layering advice report |
| `xpn_pack_revenue_estimator.py` | Model revenue projections across 3 Patreon tiers + standalone sales | Revenue projection report |
| `xpn_pack_series_planner.py` | Plan a coherent multi-pack series by analyzing existing DNA coverage gaps | Series plan (Markdown) |
| `xpn_pack_story_arc.py` | Validate narrative DNA journey within a pack | Story arc validation report |
| `xpn_collection_arc_validator.py` | Validate that a set of packs forms a coherent collection arc | Collection arc report |
| `xpn_mood_arc_designer.py` | Design mood arc progressions for XPN pack orderings | Mood arc design document |

---

## 4. Build & Export Pipeline

Scripts that build XPM programs, `.xpn` ZIP archives, or drive the full oxport pipeline.

### Core Export Pipeline

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `oxport.py` | Full XPN export pipeline orchestrator: chains all stages in one command | Complete `.xpn` archive |
| `xpn_batch_export.py` | Run the full oxport pipeline for multiple engines in sequence or parallel | Multiple `.xpn` archives |
| `xpn_xometa_export_pipeline.py` | Complete pipeline: `.xometa` preset collection → distributable `.xpn` ZIP | `.xpn` pack ZIP |
| `xpn_free_pack_pipeline.py` | Automated pipeline for building a free/entry-level gateway pack | Free `.xpn` pack |
| `xpn_packager.py` | Create MPC-loadable `.xpn` expansion archives from built program directories | `.xpn` ZIP archive |
| `xpn_submission_packager.py` | Intake normalizer and packager for community pack submissions | Normalized `.xpn` archive |

### XPM / Program Builders

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_drum_export.py` | Generate MPC-compatible drum expansion packs from XOnset presets | Drum `.xpn` pack |
| `xpn_keygroup_export.py` | Generate Akai MPC Keygroup XPM programs from WAV sample sets | Keygroup `.xpm` programs |
| `xpn_xometa_to_xpm_exporter.py` | Export `.xometa` preset files to Akai MPC Keygroup XPM programs | `.xpm` keygroup programs |
| `xpn_xpm_batch_builder.py` | Build multiple XPM programs from a single batch specification JSON | Multiple `.xpm` files |
| `xpn_articulation_builder.py` | Build XPM programs exposing multiple playing techniques across key zones | Multi-articulation `.xpm` |
| `xpn_deconstruction_builder.py` | Create a 4-bank signal chain educational XPN pack | Educational `.xpn` pack |
| `xpn_evolution_builder.py` | Generate evolution packs — a series of XPM programs showing a technique evolving | Evolution series `.xpm` files |
| `xpn_performance_map_builder.py` | Build XPM programs arranged for live performance ergonomics | Live-performance `.xpm` |
| `xpn_mpce_quad_builder.py` | Build MPCe-native feliX-Oscar quad-corner drum kits | MPCe quad `.xpm` |
| `xpn_tutorial_builder.py` | Create an educational XPN pack with 8 progressive step programs | Tutorial `.xpn` pack |
| `xpn_bundle_builder.py` | Multi-engine bundle builder with 3 modes + Collection hierarchical packaging | Bundle `.xpn` pack |

### Render Specifications

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_render_spec.py` | Read `.xometa` files and generate WAV recording specifications for producers | `render_spec.json` |
| `xpn_render_spec_generator.py` | Generate `render_spec.json` files for the oxport pipeline | `render_spec.json` |
| `xpn_complement_renderer.py` | Generate primary + complement variant XPM program pairs for color-collection engines | Complement `.xpm` pairs |

### Manifest & Metadata Builders

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_manifest_generator.py` | Generate `expansion.json` + `bundle_manifest.json` for a pack | Pack manifest JSON files |
| `xpn_expansion_json_builder.py` | Build and validate `expansion.json` — the primary MPC discovery manifest | `expansion.json` |
| `xpn_xometa_to_pack_bridge.py` | Bridge `.xometa` preset system to the XPN pack pipeline | Pack-ready metadata |
| `xpn_params_sidecar_spec.py` | Generate and validate `params_sidecar.json` for XOceanus auto-load on MPC | `params_sidecar.json` |
| `xpn_pack_registry.py` | Manage `pack_registry.json` — master catalog of all released packs | `pack_registry.json` |
| `xpn_oxport_pack_yml_template.py` | Generate `pack.yml` config for the oxport v2 pipeline | `pack.yml` template |

### Sample Processing

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_normalize.py` | Peak-normalize WAV samples for XPN/MPC pack production | Normalized WAV files |
| `xpn_smart_trim.py` | Auto-trim silence and detect loop points in WAV samples | Trimmed WAV files |
| `xpn_auto_root_detect.py` | Auto-detect fundamental pitch of a WAV and return the closest MIDI root note | MIDI root note (int) |
| `xpn_sample_loop_optimizer.py` | Analyze WAV files for optimal loop points and validate existing markers | Loop optimization report + optional edits |
| `xpn_sample_metadata_embedder.py` | Embed metadata into WAV files using RIFF LIST/INFO chunks | WAV files with embedded metadata |
| `xpn_sample_pool_builder.py` | Assemble a structured sample pool from flat raw WAV renders | Organized sample pool directory |
| `xpn_sample_rename_batch.py` | Batch-rename WAV samples inside a `.xpn` ZIP and update all XPM references | Updated `.xpn` ZIP |
| `xpn_sample_categorizer.py` | Auto-classify a folder of drum samples into voice types | Voice-labeled sample list |
| `xpn_sample_fingerprinter.py` | WAV sample deduplication and provenance tracking via perceptual hashing | Fingerprint index JSON |
| `xpn_adaptive_velocity.py` | Auto-shape velocity curves in XPM files based on instrument classification | Updated XPM velocity layers |
| `xpn_site_audio_export.py` | (see site_audio_export.py) | — |

### Pad / Macro Configuration

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_pad_note_mapper.py` | Map MPC pad positions to MIDI note numbers; validate drum kit conventions | Pad note map report |
| `xpn_pad_label_optimizer.py` | Rewrite pad labels so the 6-character MPC display shows meaningful names | Updated XPM pad labels |
| `xpn_pad_layout_visualizer.py` | Render ASCII pad layout grids from XPM drum program files | ASCII pad grid (stdout) |
| `xpn_choke_group_assigner.py` | Assign choke/mute groups to drum XPM programs based on voice classification | Updated XPM choke fields |
| `xpn_choke_group_designer.py` | Design and validate choke group assignments for a drum kit | Choke group design spec |
| `xpn_macro_assignment_suggester.py` | Suggest M1–M4 macro assignments for `.xometa` presets | Macro suggestion list |
| `xpn_macro_curve_designer.py` | Design and visualize macro response curves for XPN programs | Macro curve spec |
| `xpn_macro_label_normalizer.py` | Normalize macro labels across `.xometa` presets for consistency | Updated `.xometa` macro labels |
| `xpn_macro_performance_recorder.py` | Record and export macro performance scripts for XOceanus | Macro performance script |
| `xpn_velocity_curve_designer.py` | Generate Vibe-curve velocity boundary configs for XPM programs | Velocity config XML/JSON |
| `xpn_velocity_map_visualizer.py` | Render velocity layer structure of an XPM as an ASCII chart | ASCII velocity chart |
| `xpn_velocity_zone_visualizer.py` | Parse XPM and render each keygroup's velocity zone structure | ASCII zone visualization |
| `xpn_velocity_curve_tester.py` | Validate each keygroup's velocity layer coverage against Vibe-curve spec | Velocity validation report |

---

## 5. CI & Validation

Scripts that enforce schema correctness, run automated QA gates, and manage versioning.

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_pack_qa_ci_runner.py` | Single-command CI quality gate for the full preset fleet — unified PASS/FAIL | CI verdict report |
| `xpn_full_qa_runner.py` | Master QA orchestrator: chains all QA tools against a `.xpn` pack in sequence | Full QA report (Markdown) |
| `xpn_community_qa.py` | Headless QA orchestrator for community pack submissions | Community QA verdict |
| `xpn_pack_ci_config_generator.py` | Generate GitHub Actions `.yml` for automated `.xpn` pack quality gating | `.github/workflows/xpn_qa.yml` |
| `xpn_pack_integrity_check.py` | Deep structural integrity check on a `.xpn` pack | Integrity report |
| `xpn_validator.py` | Validate `.xometa` preset files against the XOceanus schema | Validation results |
| `xpn_manifest_validator.py` | Validate `expansion.json` and `bundle_manifest.json` — required fields, format | Validation results |
| `xpn_pack_format_validator.py` | Deep XPN/XPM format validator: MPC XML schema + compatibility checks | Format validation report |
| `xpn_pack_naming_validator.py` | Validate pack/program/sample/preset names against XO_OX brand naming rules | Naming validation report |
| `xpn_kit_validator.py` | Deep structural validation of XPM drum kit programs | Kit validation report |
| `xpn_xometa_batch_validator.py` | Validate all `.xometa` files in a directory against the XOceanus preset schema | Batch validation results |
| `xpn_xpm_validator_strict.py` | Strict XPM XML validator: 3 critical XO_OX rules + MPC format compliance | Strict validation report |
| `xpn_expansion_json_lint.py` | Lint `expansion.json` files for required fields, format compliance, suggestions | Lint results |
| `xpn_collection_arc_validator.py` | Validate that a set of packs forms a coherent collection arc | Arc validation report |
| `xpn_tide_tables_build_validator.py` | Validate a TIDE TABLES XPN pack directory against the spec | Pack spec validation report |
| `xpn_xometa_schema_version_migrator.py` | Migrate `.xometa` preset files between schema versions | Migrated `.xometa` files |
| `xpn_pack_release_checklist.py` | Run pre-release checklist verifications before packaging and publishing | Release checklist results |
| `xpn_qa_checker.py` | Perceptual quality checker for rendered WAV files in the oxport pipeline | QA pass/fail per file |

---

## 6. Creative, Release & Publishing Tools

Scripts for cover art, marketing copy, documentation, versioning, and session management.

### Cover Art & Visual

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_cover_art.py` | Generate branded procedural cover art PNG for MPC expansion packs | Cover art PNG |
| `xpn_cover_art_generator_v2.py` | Generate Gallery Model cover art PNG for a `.xpn` pack | Cover art PNG (v2 style) |
| `xpn_cover_art_batch.py` | Batch-generate cover art for multiple packs from a JSON manifest | Multiple cover art PNGs |
| `xpn_pack_thumbnail_generator.py` | Generate a simple programmatic thumbnail PNG for a pack | Thumbnail PNG |
| `xpn_pack_thumbnail_spec.py` | Generate a structured Markdown design brief for pack cover art (512×512) | Cover art design brief |

### Documentation & Marketing

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_liner_notes.py` | Auto-generate rich cultural/educational metadata for XPN packs | Liner notes document |
| `xpn_pack_marketing_copy.py` | Generate Patreon/marketplace marketing copy from pack metadata | Marketing copy (Markdown) |
| `xpn_pack_readme_generator.py` | Generate `README.md` for an XPN pack from `expansion.json` + presets | `README.md` |
| `xpn_pack_readme_updater.py` | Regenerate machine-managed sections of an existing pack `README.md` | Updated `README.md` |
| `xpn_pack_license_generator.py` | Generate `LICENSE.txt` for XO_OX XPN expansion packs | `LICENSE.txt` |
| `xpn_pack_localization_guide.py` | Check pack names and descriptions for localization issues (non-ASCII, length) | Localization issue report |
| `xpn_pack_catalog_generator.py` | Generate a human-readable catalog document for an XPN pack | Pack catalog (Markdown) |
| `xpn_pack_index_generator.py` | Generate a static `index.html` for local browsing of a pack catalog | `index.html` catalog |
| `xpn_preset_recipe_book.py` | Generate a recipe book showing how to recreate signature sounds | Recipe book document |
| `xpn_engine_tag_cloud_builder.py` | Aggregate preset tags fleet-wide and generate tag cloud data | Tag cloud JSON/Markdown |
| `xpn_preset_name_generator.py` | Generate evocative 2–3 word preset names from a 6D DNA profile | Preset name suggestions |

### Versioning & Changelog

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_pack_version_bumper.py` | Bump semantic version in a `.xpn` pack and optionally generate changelog entry | Updated `expansion.json` + changelog |
| `xpn_changelog_generator.py` | Generate `CHANGELOG.md` for an XPN expansion pack | `CHANGELOG.md` |
| `xpn_release_notes_generator.py` | Auto-generate `RELEASE_NOTES.md` for an XPN expansion pack | `RELEASE_NOTES.md` |
| `xpn_pack_changelog_tracker.py` | Compare two pack versions and generate a detailed changelog | Changelog diff (Markdown) |

### Session & Workflow

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_session_handoff.py` | Scan Tools/ and Docs/specs/ to generate a session handoff summary | Handoff document (Markdown) |
| `xpn_session_summary_generator.py` | Generate comprehensive session summary report covering all tools and specs | Session summary (Markdown) |
| `xpn_pack_size_optimizer.py` | Analyze and reduce final `.xpn` bundle size | Size optimization report |
| `xpn_pack_bundle_sizer.py` | Estimate final `.xpn` bundle size before building | Size estimate report |
| `xpn_bulk_metadata_editor.py` | Bulk-edit metadata fields across all manifest files in one or many `.xpn` archives | Updated `.xpn` manifests |
| `xpn_program_renamer.py` | Batch-rename XPM programs inside a `.xpn` ZIP archive | Updated `.xpn` ZIP |
| `xpn_preset_name_dedup_fixer.py` | Detect and fix duplicate preset names across the fleet | Renamed `.xometa` files |
| `xpn_preset_tag_normalizer.py` | Normalize tags to canonical lowercase vocabulary across `.xometa` files | Updated `.xometa` tag fields |
| `xpn_preset_tag_recommender.py` | Recommend tags for presets based on DNA, mood, and engine | Tag recommendation list |
| `xpn_preset_search_engine.py` | Full-text search across the `.xometa` preset library | Search results (stdout) |
| `xpn_preset_browser.py` | Terminal browser for exploring `.xometa` preset files interactively | Interactive terminal UI |
| `xpn_program_type_classifier.py` | Classify each XPM program in a pack as Drum, Keygroup, MIDI, or Plugin | Program type map |
| `xpn_to_sfz.py` | Convert XPN Keygroup programs to SFZ format for DAW use | `.sfz` files |
| `xpn_collection_sequencer.py` | Generate recommended release sequences for a collection of packs | Release sequence plan |
| `xpn_xometa_dna_bulk_export.py` | Export all `.xometa` DNA values to CSV, JSON, or Markdown | DNA export file |

---

## 7. Generative Kit Tools

Experimental kits where external real-world data drives MPC program structure.

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `xpn_attractor_kit.py` | Chaotic attractor velocity curves (Lorenz, Rössler) for Keygroup programs | Attractor XPN kit |
| `xpn_braille_rhythm_kit.py` | Braille dot patterns as drum rhythms | Braille XPN drum kit |
| `xpn_ca_presets_kit.py` | Wolfram 256 elementary CA rules as chromatic keygroup zones | CA keygroup XPN |
| `xpn_climate_kit.py` | Climate measurement data as velocity layers | Climate XPN kit |
| `xpn_entropy_kit.py` | Shannon entropy and information theory as velocity/timing structure | Entropy XPN kit |
| `xpn_gene_kit.py` | DNA codon sequences → MPC-compatible drum programs | Genomic XPN drum kit |
| `xpn_git_kit.py` | Git commit history → MPC drum patterns | Git history XPN kit |
| `xpn_gravitational_wave_kit.py` | GW150914 chirp waveform → Keygroup programs | Gravitational wave XPN kit |
| `xpn_lsystem_kit.py` | Fractal L-system string rewriting → drum patterns | L-system XPN kit |
| `xpn_pendulum_kit.py` | Huygens coupled-pendulum synchronization → rhythm structures | Pendulum XPN kit |
| `xpn_poetry_kit.py` | CMU Pronouncing Dictionary stress patterns → drum rhythms | Poetry XPN drum kit |
| `xpn_seismograph_kit.py` | USGS earthquake data → MPC drum kit | Seismic XPN kit |
| `xpn_transit_kit.py` | City transit schedules as polyrhythmic drum kits | Transit XPN kit |
| `xpn_turbulence_kit.py` | Kolmogorov -5/3 turbulence energy cascade → velocity layers | Turbulence XPN kit |
| `xpn_curiosity_engine.py` | Happy Accident Machine — randomized generative preset discovery | Random `.xometa` stubs |
| `xpn_monster_rancher.py` | Breed XPN programs from cross-engine DNA hybridization | Hybrid XPN programs |
| `xpn_setlist_builder.py` | Generate an ordered setlist of XPN packs optimized for live performance | Live setlist plan |
| `xpn_stem_pack_designer.py` | Design stem-aware pack structures for multi-track MPC workflows | Stem pack spec |
| `xpn_tide_tables_pack_builder.py` | Build the TIDE TABLES free gateway pack spec for XO_OX Patreon | TIDE TABLES pack spec |
| `xpn_quick_preview_generator.py` | Generate quick preview audio specs for pack demos | Preview spec |
| `xpn_preview_generator.py` | Generate preview audio specs and metadata for pack demonstrations | Preview package |

---

## 8. Legacy & Early-Generation Utilities

Older scripts that seeded the initial preset library or performed one-time migrations.

| Filename | One-Line Purpose | Key Output |
|---|---|---|
| `validate_presets.py` | Comprehensive `.xometa` QA: schema, DNA, coupling route validation | Validation report |
| `compute_preset_dna.py` | Compute 6D DNA vector for every `.xometa` preset and write back | Updated `.xometa` DNA blocks |
| `audit_sonic_dna.py` | Audit 6D DNA coverage per engine; find empty or homogeneous engines | DNA audit report |
| `find_missing_dna.py` | Find all `.xometa` presets missing a `sonic_dna` block | Missing DNA file list |
| `add_missing_dna.py` | Auto-populate missing `sonic_dna` blocks with engine-inferred defaults | Updated `.xometa` files |
| `breed_presets.py` | Genetic preset breeder: crossover + mutation between two parents | Bred `.xometa` presets |
| `extract_cpp_presets.py` | Extract factory presets from XOverdub, XOdyssey, XOblong C++ sources | Extracted `.xometa` presets |
| `generate_coupling_presets.py` | Early coupling coverage fill targeting Overworld/Overbite/Organon/Ouroboros | `.xometa` coupling presets |
| `generate_library_fills.py` | Fill mood gaps in Overdub/Odyssey/OddOscar/ONSET/OddfeliX | `.xometa` preset stubs |
| `generate_preset_params.py` | Generate DSP parameter values for `.xocmeta` presets from metadata | Updated `.xocmeta` params |
| `migrate_xocmeta_to_xometa.py` | One-time migration: OddfeliX `.xocmeta` → XOceanus `.xometa` format | `.xometa` preset files |
| `apply_renames.py` | Bulk rename preset files, docs, and tools in a single pass | Renamed files |
| `rename_weak_presets.py` | Elevate generic/functional preset names to evocative alternatives fleet-wide | Renamed `.xometa` files |
| `fix_bob_presets.py` | Migrate XOblong preset params to canonical BobEngine schema | Fixed `.xometa` files |
| `fix_drift_presets.py` | Migrate XOdyssey preset params to canonical DriftEngine schema | Fixed `.xometa` files |
| `fix_dub_presets.py` | Migrate XOverdub preset params to canonical DubEngine schema | Fixed `.xometa` files |
| `fix_overworld_presets.py` | Fix Overworld presets using UPPER_SNAKE_CASE param keys → canonical casing | Fixed `.xometa` files |
| `fix_xobese_xpms.py` | Patch broken XObese keygroup programs to match MPC conventions | Fixed `.xpm` files |
| `site_audio_export.py` | Process raw WAV recordings from synth plugins into web-ready audio | Web-ready WAV/MP3 files |
| `generate_constellation_presets.py` | Generate 530 `.xometa` presets for the 5 Constellation engines | Constellation `.xometa` presets |
| `generate_onset_presets.py` | Generate XOnset factory `.xometa` presets | XOnset `.xometa` presets |
| `generate_opal_presets.py` | Generate 100 XOpal factory presets across 5 granular categories | XOpal `.xometa` presets |
| `generate_organon_presets.py` | Generate 120 XOrganon factory presets | XOrganon `.xometa` presets |
| `generate_organon_coupling_presets.py` | Generate Organon + partner engine coupling presets | Coupling `.xometa` presets |
| `generate_ouroboros_presets.py` | Generate XOuroboros presets across 4 chaotic attractor topologies | XOuroboros `.xometa` presets |
| `generate_overbite_presets.py` | Generate XOverbite factory presets for bass-forward character engine | XOverbite `.xometa` presets |
| `generate_overworld_presets.py` | Generate XOverworld presets across chip-synthesis eras | XOverworld `.xometa` presets |
| `benchmark_organon.cpp` | C++ microbenchmark for XOrganon DSP performance | Benchmark binary |

---

## Fleet DNA Status

### Coupling Coverage
- **Total pairs covered**: 174 / 561 possible engine pairs (**31.0%**)
- **Engines with lowest coverage** (highest priority for expansion):
  - OCTOPUS — 2 partner pairings
  - OMBRE — 2 partner pairings
  - ORCA — 3 partner pairings
  - OSTERIA — 3 partner pairings
  - OWLFISH — 4 partner pairings

### DNA Dimension Compression
The 6D Sonic DNA fleet distribution shows these dimensions are most compressed (clustered toward center, lacking extremes):

| Dimension | Compression Score |
|---|---|
| Brightness | 85.4% |
| Warmth | 80.7% |
| Density | 77.7% |

The hot/cold/sparse/dense quadrant packs and the extreme brightness packs were generated specifically to address these compressions.

### Next Priority Actions
1. **OCTOPUS / OMBRE expansion** — `xpn_ombre_orca_octopus_coupling_pack.py` and `xpn_orca_octopus_ombre_trio_pack.py` partially address this; more partner pairings needed
2. **ORCA / OSTERIA expansion** — `xpn_osprey_osteria_coupling_pack.py` covers OSTERIA; ORCA needs a dedicated pass
3. **OWLFISH expansion** — `xpn_owlfish_coupling_pack.py` targets this directly (12 partners, 36 presets); verify it ran and committed
4. **Brightness push** — run `xpn_extreme_brightness_v2_pack.py` on remaining engines below 0.3 brightness centroid
5. **Density extremes** — run `xpn_cold_dense_preset_pack.py` and `xpn_hot_dense_preset_pack.py` for engines with density centroid in 0.4–0.6 band

---

*Generated by XPN Tool Suite session — XO_OX Designs*
