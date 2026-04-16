# XPN Tool Suite — Complete Developer Reference

**Location:** `Tools/` (repo root)
**Total tools:** 270 xpn_*.py + 28 supporting scripts
**Last updated:** 2026-03-16

---

## 1. Overview

The XPN Tool Suite is a pure-Python (stdlib only) collection of scripts that covers the full lifecycle of XOceanus preset development and MPC expansion pack production:

- **Preset authoring** — generate `.xometa` stub presets for all mood categories, coupling pairs, and DNA zones
- **Coverage tracking** — audit which engine pairs, moods, and DNA regions are under-represented
- **DNA analysis** — measure 6D sonic diversity, find compressed zones, prescribe targeted fills
- **Export pipeline** — render `.xometa` → render spec → WAV samples → `.xpm` programs → `.xpn` ZIP archive
- **Quality assurance** — validate format compliance, perceptual audio checks, deduplication

All tools run from the repo root with no external dependencies:

```sh
python3 Tools/xpn_<tool_name>.py [args]
```

---

## 2. Core Concepts

### .xometa Format

`.xometa` files are JSON preset descriptors stored in `Presets/XOceanus/{Mood}/`. Every preset requires:

| Field | Type | Description |
|---|---|---|
| `name` | string | 2–3 words, ≤30 chars, no jargon |
| `engine` | string | Canonical engine name (e.g. `"OPAL"`) |
| `mood` | string | One of 7 moods (see below) |
| `sonic_dna` | object | 6D DNA vector (see below) |
| `macro_labels` | array[4] | Exactly 4 non-empty strings |
| `tags` | array | Genre/character descriptors |
| `coupling` | object | Optional: `engines`, `type`, `amount` |
| `params` | object | Engine parameter snapshot (engine-prefixed keys) |
| `version` | string | Schema version (current: `"1.1"`) |

### 6D Sonic DNA

Every preset carries a `sonic_dna` block with six normalized dimensions (0.0–1.0):

| Dimension | Lo (0.0) | Hi (1.0) |
|---|---|---|
| `brightness` | dark, murky | bright, crystalline |
| `warmth` | cold, digital | warm, analog |
| `movement` | static, frozen | kinetic, chaotic |
| `density` | sparse, open | dense, saturated |
| `space` | dry, contained | spacious, reverberant |
| `aggression` | gentle, passive | aggressive, distorted |

Presets with ≥2 dimensions ≤0.15 or ≥0.85 are **extreme-zone** anchors — crucial for fleet diversity. The target diversity score (mean cosine distance between preset pairs) is **≥0.35**. Current fleet: **0.1535** (brightness compression is the primary problem).

### Coupling Types

Entangled mood presets reference two or more engines via a `coupling` block. Valid `type` values:

`TIMBRE_BLEND`, `PHASE_LOCK`, `HARMONIC_SYNC`, `RHYTHM_LOCK`, `SPECTRAL_BLEND`, `ENVELOPE_FOLLOW`, `PITCH_TRACK`, `FORMANT_BLEND`, `AMPLITUDE_LOCK`, `TEXTURE_WEAVE`, `RESONANCE_SHARE`, `CHAOS_SYNC`

### 7 Mood Categories

| Mood | Character |
|---|---|
| Foundation | Grounded, workhorse sounds — dense, warm, reliable |
| Atmosphere | Textural, ambient — drift, space, movement |
| Entangled | Coupling presets — two or more engines interacting |
| Prism | Bright, spectral, crystalline — high brightness |
| Flux | Kinetic, aggressive — high movement/aggression |
| Aether | Extreme space, dark or transcendent — high space |
| Family | Engine portraits and duo/trio relationships |

### Engine Parameter Prefixes (canonical)

Each engine uses a prefix for all parameter keys in the `params` block. Key prefixes:

| Engine | Prefix | Engine | Prefix |
|---|---|---|---|
| OBLONG (BOB) | `bob_` | OPAL | `opal_` |
| OBESE | `fat_` | ORBITAL | `orb_` |
| OVERBITE | `poss_` | ORGANON | `org_` |
| OVERDUB | `dub_` | OUROBOROS | `ouro_` |
| ODYSSEY | `drift_` | OBSIDIAN | `obsidian_` |
| ONSET | `perc_` | ORIGAMI | `orig_` |
| OVERWORLD | `ow_` | ORACLE | `ora_` |
| OHM | `ohm_` | OBSCURA | `osc_` |
| ORPHICA | `orph_` | OCEANIC | `ocean_` |
| OBBLIGATO | `obbl_` | OCELOT | `ocelot_` |
| OTTONI | `otto_` | OPTIC | `optic_` |
| OLE | `ole_` | OBLIQUE | `obq_` |
| OVERLAP | `olap_` | OSPREY | `osprey_` |
| OUTWIT | `owit_` | OSTERIA | `osteria_` |
| OMBRE | `ombr_` | OWLFISH | `owlfish_` |
| ORCA | `orca_` | OCTOPUS | `oct_` |
| ODDFELIX / ODDOSCAR | `odd_` | | |

---

## 3. Tool Index

### 3.1 Analytics and Health

| Tool | Purpose | Output |
|---|---|---|
| `xpn_fleet_health_dashboard.py` | Combined single-pass: coupling coverage + mood distribution + DNA diversity | `Docs/snapshots/fleet_health_latest.json` + stdout |
| `xpn_fleet_health_summary.py` | Columnar text fleet report (presets per engine/mood, tag clouds, duplicates) | stdout or `--output` file |
| `xpn_fleet_diversity_score_v2.py` | DNA diversity score, midrange compression %, histograms per dimension | `Docs/snapshots/diversity_score_latest.json` |
| `xpn_fleet_dna_diversity_report.py` | Deep diagnosis: WHY diversity is low, prescribes targeted fills | stdout |
| `xpn_dna_diversity_analyzer.py` | Fleet DNA diversity problems report + per-engine dimension stats | `Docs/snapshots/dna_diversity_analysis.json` |
| `xpn_coupling_coverage_tracker_v2.py` | Coupling pair coverage matrix for all 34 engines (561 total pairs) | `Docs/snapshots/coupling_coverage_latest.json` |
| `xpn_coupling_density_heatmap.py` | ASCII heatmap of engine×engine coupling coverage | stdout |
| `xpn_engine_coverage_mapper.py` | Per-engine coverage map, gaps, recommended pack priorities | stdout or `--output` |
| `xpn_engine_preset_gap_reporter.py` | Per-engine gap report: missing DNA zones, mood gaps, fill prescriptions | stdout or `--output` |
| `xpn_engine_dna_profile_builder.py` | Authoritative 6D DNA fingerprint per engine (mean/min/max/std) | `Docs/engine_dna_profiles.json` |
| `xpn_dna_gap_finder.py` | Underrepresented 6D DNA regions across the fleet | stdout or `--output` |
| `xpn_pack_analytics.py` | Analytics over a directory of released `.xpn` ZIPs | text/json/markdown |
| `xpn_expansion_bundle_profiler.py` | Bundle fingerprint for a single `.xpn` ZIP (QA + release comparison) | stdout or `--output` |
| `xpn_preset_clustering_report.py` | K-means style DNA cluster analysis, labels high-density zones | stdout |
| `xpn_preset_fleet_snapshot.py` | Point-in-time snapshot JSON of the full fleet state | `Docs/snapshots/fleet_YYYYMMDD.json` |
| `xpn_preset_variety_scorer.py` | Per-engine variety score based on DNA spread | stdout |
| `xpn_preset_similarity_matrix.py` | Pairwise DNA similarity matrix, highlights near-duplicates | stdout |

### 3.2 Coverage and Coupling Pack Generators

These tools generate `.xometa` stub presets in `Presets/XOceanus/Entangled/`. They all skip existing files.

**Engine-specific coupling packs:**

| Tool | Engines Covered |
|---|---|
| `xpn_constellation_coupling_pack.py` | OHM, ORPHICA, OBBLIGATO, OTTONI, OLE — all 10 intra-Constellation pairs + 5 legacy bridges |
| `xpn_constellation_complete_coverage_pack.py` | OHM and OBBLIGATO gap closure (72 presets, 6 partners each) |
| `xpn_constellation_external_coupling_pack.py` | Constellation engines × all non-Constellation legacy partners |
| `xpn_dark_engines_coupling_pack.py` | OBESE, OVERBITE, OBSIDIAN, OUROBOROS — intra-group + external partners (~56 presets) |
| `xpn_rhythm_engines_coupling_pack.py` | OBLONG, ONSET, OVERWORLD — rhythm/chip trio (~66 presets) |
| `xpn_shore_engines_coupling_pack.py` | OSPREY, OSTERIA, OCEANIC — shore trinity (~60 presets) |
| `xpn_legacy_coupling_pack.py` | 20 highest-affinity uncovered legacy pairs (DNA complementarity ranked) |
| `xpn_obese_gap_closure_pack.py` | All missing OBESE coupling partners |
| `xpn_overbite_gap_closure_pack.py` | All missing OVERBITE coupling partners (auto-scans, generates 2 per gap) |
| `xpn_outwit_overlap_coupling_pack.py` | OUTWIT × OVERLAP pair |
| `xpn_overlap_outwit_coupling_pack.py` | OVERLAP × OUTWIT (extended variant) |
| `xpn_overlap_outwit_deep_expansion.py` | Deep 50-preset expansion for OVERLAP/OUTWIT pairs |
| `xpn_obbligato_coupling_pack.py` | OBBLIGATO partner coverage |
| `xpn_ohm_orphica_ottoni_ole_coupling_pack.py` | Constellation intra-group pack |
| `xpn_overworld_oblong_coupling_pack.py` | OVERWORLD × OBLONG |
| `xpn_overdub_opal_coupling_pack.py` | OVERDUB × OPAL |
| `xpn_oracle_odyssey_coupling_pack.py` | ORACLE × ODYSSEY |
| `xpn_organon_ouroboros_coupling_pack.py` | ORGANON × OUROBOROS |
| `xpn_ouroboros_organon_deep_coupling_pack.py` | Deep OUROBOROS/ORGANON expansion |
| `xpn_oblong_onset_coupling_pack.py` | OBLONG × ONSET |
| `xpn_obese_overbite_coupling_pack.py` | OBESE × OVERBITE |
| `xpn_obsidian_obscura_coupling_pack.py` | OBSIDIAN × OBSCURA |
| `xpn_obsidian_oracle_coupling_pack.py` | OBSIDIAN × ORACLE |
| `xpn_oceanic_ocelot_coupling_pack.py` | OCEANIC × OCELOT |
| `xpn_ocelot_orbital_coupling_pack.py` | OCELOT × ORBITAL |
| `xpn_octopus_ombre_deep_coverage_pack.py` | OCTOPUS × OMBRE deep |
| `xpn_oddfelix_oddoscar_coupling_pack.py` | ODDFELIX × ODDOSCAR |
| `xpn_odyssey_oblong_obese_coverage_pack.py` | ODYSSEY × OBLONG × OBESE trio |
| `xpn_ombre_orca_octopus_coupling_pack.py` | OMBRE × ORCA × OCTOPUS |
| `xpn_ombre_orca_octopus_presets.py` | OMBRE/ORCA/OCTOPUS preset stubs |
| `xpn_optic_oblique_ocelot_coupling_pack.py` | OPTIC × OBLIQUE × OCELOT |
| `xpn_orca_octopus_ombre_trio_pack.py` | ORCA/OCTOPUS/OMBRE trio |
| `xpn_orca_osteria_owlfish_coverage_pack.py` | ORCA × OSTERIA × OWLFISH |
| `xpn_origami_optic_oblique_coupling_pack.py` | ORIGAMI × OPTIC × OBLIQUE |
| `xpn_obscura_ocelot_oblique_coverage_pack.py` | OBSCURA × OCELOT × OBLIQUE |
| `xpn_obscura_origami_orbital_coupling_pack.py` | OBSCURA × ORIGAMI × ORBITAL |
| `xpn_osprey_osteria_coupling_pack.py` | OSPREY × OSTERIA |
| `xpn_ottoni_ole_orphica_coverage_pack.py` | OTTONI × OLE × ORPHICA coverage |
| `xpn_owlfish_coupling_pack.py` | OWLFISH partner coverage |
| `xpn_orbital_overbite_obese_pack.py` | ORBITAL × OVERBITE × OBESE |

**General gap closure:**

| Tool | Purpose |
|---|---|
| `xpn_coverage_final_sprint.py` | Scan Entangled, find ALL uncovered pairs (from 34-engine roster), generate 2 presets per gap |
| `xpn_fleet_coupling_coverage_updater.py` | Incremental updater — runs after any coupling pack to refresh the coverage snapshot |
| `xpn_coupling_recipe_expander.py` | Expand existing coupling recipes with new DNA variations |
| `xpn_coupling_pair_recommender.py` | Recommend best next pairs based on DNA affinity + coverage gaps |
| `xpn_flagship_trio_deep_expansion.py` | Deep 50-preset expansion for the founding trio (ODDFELIX/ODDOSCAR/OVERDUB) |
| `xpn_founding_pair_deep_expansion.py` | Deep expansion for the founding pair |

### 3.3 Mood Expansion Packs

Generate `.xometa` presets for non-Entangled moods. All output to `Presets/XOceanus/{Mood}/`, skip existing files.

| Tool | Mood | Count | Sub-themes |
|---|---|---|---|
| `xpn_foundation_expansion_pack.py` | Foundation | 60 | Bedrock Bass / Anchor Tone / Work Horse |
| `xpn_atmosphere_expansion_pack.py` | Atmosphere | 60 | Dawn / Deep / Electric / Warm Drone |
| `xpn_prism_expansion_pack.py` | Prism | 60 | Spectral / Refraction / Crystal |
| `xpn_flux_aggression_pack.py` | Flux | 60 | Maximum Flux / Controlled Flux / Tension Flux |
| `xpn_flux_dna_expansion_pack.py` | Flux | 60 | High-flux / Cold-flux / Dense-flux / Sparse-flux |
| `xpn_aether_expansion_pack.py` | Aether | 60 | Transcendent / Dark Aether / Shifting Aether |
| `xpn_family_expansion_pack.py` | Family | 60 | Engine portraits / extended family / reunion duos |
| `xpn_family_mood_expander.py` | Family | varies | Additional family mood fills |
| `xpn_family_portrait_generator.py` | Family | varies | Individual engine portrait presets |
| `xpn_foundation_prism_diversity_anchors.py` | Foundation+Prism | varies | Extreme-zone anchors for both moods |
| `xpn_movement_aggression_expander.py` | mixed | varies | High movement + aggression cross-mood fills |
| `xpn_space_dimension_expander.py` | mixed | varies | High space presets across moods |
| `xpn_space_expander.py` | mixed | varies | Space dimension gap filler |
| `xpn_warmth_cold_dense_expander.py` | mixed | varies | Warmth/cold/dense dimension fills |
| `xpn_aggression_expansion_pack.py` | mixed | varies | Fleet-wide aggression dimension expansion |
| `xpn_brightness_expansion_pack.py` | mixed | varies | Fleet-wide brightness extreme fills |

**Entangled sub-mood packs** (also go to `Entangled/`):

| Tool | Sub-theme |
|---|---|
| `xpn_entangled_brutal_aggression_pack.py` | Extreme aggression coupling presets |
| `xpn_entangled_dark_brightness_pack.py` | Dark/low-brightness coupling presets |
| `xpn_entangled_high_variance_pack.py` | High DNA variance coupling presets |
| `xpn_entangled_hot_warmth_pack.py` | High warmth coupling presets |
| `xpn_entangled_preset_fixer.py` | Fix malformed Entangled presets in-place |

### 3.4 DNA Repair and Extreme-Zone Tools

Use these when the diversity score or midrange compression % needs improvement.

| Tool | Purpose |
|---|---|
| `xpn_dna_extreme_zone_injector.py` | Generate 80 presets anchoring all 8 corners of the 6D hypercube |
| `xpn_preset_dna_extreme_filler.py` | Smart dispatcher: diagnoses most-compressed dimensions, auto-generates targeted fills |
| `xpn_extreme_brightness_v2_pack.py` | Extreme brightness anchors (both dark ≤0.1 and bright ≥0.9) |
| `xpn_cold_dense_preset_pack.py` | Cold (brightness ≤0.2) + dense (density ≥0.8) presets |
| `xpn_cold_sparse_preset_pack.py` | Cold + sparse (density ≤0.2) presets |
| `xpn_hot_dense_preset_pack.py` | Hot (brightness ≥0.8) + dense presets |
| `xpn_hot_sparse_preset_pack.py` | Hot + sparse presets |
| `xpn_preset_dna_fixer.py` | Repair malformed or missing DNA blocks in existing `.xometa` files |
| `xpn_preset_dna_recalibrator.py` | Re-derive DNA from parameter values when manual DNA is suspect |
| `xpn_sonic_dna_interpolator_v2.py` | Generate NEW preset stubs at interpolated DNA positions between two poles |
| `xpn_dna_interpolator.py` | Find nearest existing presets along a DNA path between two endpoints |
| `xpn_preset_rebalancer.py` | Shift DNA values to improve fleet balance (non-destructive, dry-run safe) |
| `xpn_preset_mood_rebalancer.py` | Re-assign mood tags to improve 7-mood balance |

**Supporting non-xpn_ DNA scripts:**

| Tool | Purpose |
|---|---|
| `compute_preset_dna.py` | Compute/recompute DNA for all presets from parameter values |
| `audit_sonic_dna.py` | Audit per-engine DNA coverage: missing blocks, under/overrepresented extremes |
| `add_missing_dna.py` | Batch-add minimal DNA blocks to presets that lack them |
| `find_missing_dna.py` | List all presets missing a `sonic_dna` block |

### 3.5 Export Pipeline

The full MPC export path: `.xometa` presets → WAV samples → `.xpm` programs → `.xpn` archive.

**Orchestrators (run these first):**

| Tool | Purpose |
|---|---|
| `oxport.py` | Master pipeline orchestrator: render_spec → categorize → expand → QA → export → cover_art → package |
| `xpn_xometa_export_pipeline.py` | Lighter pipeline: `.xometa` collection → `.xpm` stubs → ZIP (no audio rendering) |
| `xpn_batch_export.py` | Batch orchestrator: runs oxport for multiple engines in sequence or parallel |

**Stage 1 — Render specification:**

| Tool | Purpose | Output |
|---|---|---|
| `xpn_render_spec.py` | Read `.xometa`, output render spec (MIDI notes, velocities, durations per engine) | JSON spec |
| `xpn_render_spec_generator.py` | Batch render spec generator for a filtered set of presets | `render_spec.json` |

**Stage 2 — Sample preparation:**

| Tool | Purpose |
|---|---|
| `xpn_sample_categorizer.py` | Auto-classify WAV files into kick/snare/hat/clap/tom/perc/fx using filename keywords or audio analysis |
| `xpn_kit_expander.py` | Expand flat single-sample kits → velocity layers (v1–v4) or round-robin variants (c1–c4) via DSP transforms |
| `xpn_adaptive_velocity.py` | Auto-derive velocity layer boundaries from sample dynamics |
| `xpn_smart_trim.py` | Trim silence/pad from WAV files to spec |
| `xpn_sample_rate_auditor.py` | Check WAVs for sample rate consistency |
| `xpn_sample_loop_checker.py` | Verify loop points are set correctly |
| `xpn_sample_loop_optimizer.py` | Optimize loop points for seamless playback |

**Stage 3 — Program generation:**

| Tool | Purpose | Output |
|---|---|---|
| `xpn_drum_export.py` | Generate MPC `<Program type="Drum">` XPM — 8 pads, velocity/cycle/random/smart kit modes | `.xpm` |
| `xpn_keygroup_export.py` | Generate MPC `<Program type="Keygroup">` XPM — chromatically mapped velocity layers | `.xpm` |
| `xpn_xometa_to_xpm_exporter.py` | Export `.xometa` as parameter-snapshot Keygroup XPM (Q-Link macros + base64 preset embed) | `.xpm` |
| `xpn_xpm_batch_builder.py` | Batch `.xpm` builder across multiple preset sets | multiple `.xpm` |
| `xpn_xpm_template_library.py` | Reusable XPM template fragments (drum pad defaults, keygroup defaults) | library module |

**Stage 4 — Cover art:**

| Tool | Purpose |
|---|---|
| `xpn_cover_art_generator_v2.py` | Pure-stdlib PNG cover art: flag layout, engine accent color, DNA dots strip (400×400 or 800×800) |
| `xpn_cover_art.py` | Earlier version cover art generator |
| `xpn_cover_art_batch.py` | Batch generate cover art for all presets in a directory |
| `xpn_cover_art_audit.py` | Audit which packs are missing cover art |

**Stage 5 — Packaging:**

| Tool | Purpose | Output |
|---|---|---|
| `xpn_packager.py` | Create MPC-loadable `.xpn` ZIP archive with correct Expansions/Programs/Samples structure | `.xpn` |
| `xpn_bundle_builder.py` | Flexible multi-engine pack builder: custom / category / engine / collection modes | `.xpn` |
| `xpn_manifest_generator.py` | Generate `expansion.json` + `bundle_manifest.json` for a pack | two JSON files |

### 3.6 Validation and QA

| Tool | Purpose |
|---|---|
| `xpn_validator.py` | Lint `.xpn` ZIP archives against Rex's XPN Bible: structure, manifest, XPM XML, WAV integrity |
| `xpn_manifest_validator.py` | Validate `expansion.json` and `bundle_manifest.json` (fields, ranges, cross-file consistency) |
| `xpn_kit_validator.py` | Validate drum `.xpm`: pad count, velocity layers, mute groups |
| `xpn_xometa_batch_validator.py` | Batch validate all `.xometa` files against XOceanus preset schema |
| `xpn_xometa_completeness_auditor.py` | 12-dimension quality score per `.xometa` file; flag low-scorers |
| `xpn_qa_checker.py` | Perceptual WAV QA: silence, clipping, DC offset, phase cancellation, undynamic |
| `xpn_full_qa_runner.py` | Master QA orchestrator: chains manifest + sample + cover art + dedup + kit validators |
| `xpn_pack_integrity_check.py` | File-level integrity check on a `.xpn` archive |
| `xpn_pack_format_validator.py` | Format compliance check (MPC OS compatibility) |
| `xpn_xpm_validator_strict.py` | Strict XPM XML schema validation |
| `xpn_expansion_json_lint.py` | Lint `expansion.json` format |
| `xpn_dedup_checker.py` | Find duplicate WAV files inside a `.xpn` or directory |
| `xpn_preset_duplicate_detector.py` | Find near-duplicate `.xometa` presets by DNA distance |
| `xpn_preset_dedup_finder.py` | Name-based duplicate preset finder |
| `validate_presets.py` | Comprehensive `.xometa` QA: schema, DNA, macros, engine names, coupling, naming rules |

### 3.7 Special Packs

| Tool | Purpose |
|---|---|
| `xpn_tide_tables_pack_builder.py` | Build spec + README for TIDE TABLES (free Patreon gateway pack: SURGE/DRIFT/CURRENT/SWELL) |
| `xpn_tide_tables_build_validator.py` | Validate a TIDE TABLES pack directory against spec |
| `xpn_free_pack_pipeline.py` | Full pipeline for free/gateway packs |
| `xpn_engine_intro_preset_builder.py` | Generate placeholder presets for concept engines without source code yet (OSTINATO, OPENSKY, OCEANDEEP, OUIE) |
| `xpn_flagship_trio_deep_expansion.py` | 50-preset deep expansion for founding trio engines |
| `xpn_collection_arc_validator.py` | Validate a collection (Kitchen/Travel/Artwork) arc for completeness |
| `xpn_collection_sequencer.py` | Sequence a collection's presets into a logical arc |
| `xpn_mpce_quad_builder.py` | Build a 4-engine quad pack for the MPCE format |
| `xpn_complement_renderer.py` | Generate complementary color/DNA pairs for Artwork collection |

### 3.8 Preset Authoring Utilities

| Tool | Purpose |
|---|---|
| `xpn_preset_batch_generator.py` | Generate preset stubs in batch from a template config |
| `xpn_preset_variation_generator.py` | Create N variations of an existing preset by mutating DNA + params |
| `xpn_variation_generator.py` | Lower-level variation generator |
| `xpn_preset_name_generator.py` | Generate on-brand 2–3 word preset names from a DNA profile |
| `xpn_preset_name_dedup_fixer.py` | Rename duplicate preset names to unique variants |
| `xpn_preset_emotion_wheel.py` | Map presets onto the emotion wheel by DNA profile |
| `xpn_preset_mood_classifier.py` | Auto-classify a preset's mood from its DNA |
| `xpn_preset_crossfade_designer.py` | Design smooth crossfade paths between two presets |
| `xpn_preset_evolution_tracker.py` | Track DNA drift across versions of the same preset |
| `xpn_preset_aging_simulator.py` | Simulate how a sound "ages" over generative mutations |
| `xpn_preset_recipe_book.py` | Curated collection of DNA recipes for common sound characters |
| `xpn_preset_search_engine.py` | Search presets by DNA similarity, mood, engine, or tag |
| `xpn_preset_browser.py` | Interactive CLI preset browser |
| `xpn_preset_tag_normalizer.py` | Normalize and deduplicate tags across the fleet |
| `xpn_preset_tag_recommender.py` | Suggest tags for a preset based on its DNA and engine |
| `xpn_bulk_metadata_editor.py` | Batch-edit metadata fields across many `.xometa` files |
| `xpn_normalize.py` | Normalize DNA values or parameter ranges across a preset set |
| `xpn_macro_assignment_suggester.py` | Suggest macro label assignments based on engine character |
| `xpn_macro_coverage_checker.py` | Audit macro label completeness across the fleet |
| `xpn_macro_label_normalizer.py` | Normalize macro label capitalization and style |
| `xpn_macro_curve_designer.py` | Design macro response curves |

### 3.9 Kit and Pad Utilities

| Tool | Purpose |
|---|---|
| `xpn_kit_builder_assistant.py` | Interactive kit construction assistant |
| `xpn_kit_completeness.py` | Check a kit for missing voices (all 8 pads covered?) |
| `xpn_kit_layering_advisor.py` | Advise on layering strategy given a set of samples |
| `xpn_kit_theme_analyzer.py` | Analyze the thematic consistency of a kit |
| `xpn_choke_group_assigner.py` | Assign MPC choke groups (e.g. open hat muted by closed hat) |
| `xpn_choke_group_designer.py` | Design choke group logic for complex kits |
| `xpn_pad_label_optimizer.py` | Optimize pad labels for MPC display (16-char limit) |
| `xpn_pad_layout_visualizer.py` | ASCII visualization of pad layout |
| `xpn_pad_note_mapper.py` | Map MIDI notes to pad positions |
| `xpn_performance_map_builder.py` | Build MPC performance maps (Q-Link → param assignments) |
| `xpn_velocity_curve_designer.py` | Design velocity response curves per pad/voice |
| `xpn_velocity_curve_tester.py` | Test velocity curve assignments |
| `xpn_velocity_map_visualizer.py` | Visualize velocity layer mappings |
| `xpn_velocity_zone_visualizer.py` | Show velocity zones on a keyboard diagram |
| `xpn_layer_balance_checker.py` | Check that velocity layer RMS levels are balanced |

### 3.10 Release and Publishing Utilities

| Tool | Purpose |
|---|---|
| `xpn_manifest_generator.py` | Generate `expansion.json` + `bundle_manifest.json` |
| `xpn_pack_catalog_generator.py` | Generate a product catalog from all packs |
| `xpn_pack_index_generator.py` | Build a searchable pack index |
| `xpn_pack_readme_generator.py` | Auto-generate a README for a pack |
| `xpn_pack_readme_updater.py` | Update existing pack READMEs |
| `xpn_release_notes_generator.py` | Generate release notes from git diff + preset delta |
| `xpn_changelog_generator.py` | Generate a changelog |
| `xpn_pack_version_bumper.py` | Bump version in manifest + filenames |
| `xpn_pack_release_checklist.py` | Pre-release checklist runner |
| `xpn_liner_notes.py` | Generate liner notes / story text for a pack |
| `xpn_pack_marketing_copy.py` | Generate marketing copy from pack metadata |
| `xpn_pack_story_arc.py` | Define thematic story arc for a pack |
| `xpn_pack_series_planner.py` | Plan a multi-release pack series |
| `xpn_pack_score.py` | Score a pack against quality/diversity rubric |
| `xpn_pack_compare_report.py` | Compare two pack versions (preset delta, DNA shift) |
| `xpn_pack_diff.py` | File-level diff of two `.xpn` archives |
| `xpn_pack_bundle_sizer.py` | Estimate final pack file size before build |
| `xpn_pack_size_optimizer.py` | Identify oversized samples, recommend trim points |
| `xpn_pack_diet_analyzer.py` | Analyze pack for redundant/low-value content |
| `xpn_pack_license_generator.py` | Generate license file for a pack |
| `xpn_pack_thumbnail_generator.py` | Generate thumbnail image for pack listing |
| `xpn_pack_thumbnail_spec.py` | Thumbnail spec helper |
| `xpn_pack_localization_guide.py` | Localization guidance for international releases |
| `xpn_pack_revenue_estimator.py` | Revenue projection from pack pricing + projected units |
| `xpn_pack_discovery_optimizer.py` | Optimize pack metadata for discoverability |

### 3.11 Specialty and Research Tools

| Tool | Purpose |
|---|---|
| `xpn_coupling_recipes.py` | Curated coupling recipe library (engine pair → recommended type + DNA) |
| `xpn_coupling_recipe_expander.py` | Expand recipes into full preset stubs |
| `xpn_coupling_preset_auditor.py` | Audit quality of existing coupling presets |
| `xpn_auto_dna.py` | Auto-derive DNA from parameter values (engine-specific heuristics) |
| `xpn_auto_root_detect.py` | Auto-detect root note of a sample |
| `xpn_program_renamer.py` | Batch rename `.xpm` programs |
| `xpn_program_type_classifier.py` | Classify `.xpm` as DrumProgram or KeygroupProgram |
| `xpn_optic_fingerprint.py` | OPTIC engine visual fingerprint (waveform identity) |
| `xpn_engine_tag_cloud_builder.py` | Build tag clouds per engine from preset tags |
| `xpn_tuning_coverage_checker.py` | Check tuning system coverage across presets |
| `xpn_tuning_systems.py` | Tuning system library (Just, Pythagorean, Equal, etc.) |
| `xpn_to_sfz.py` | Convert `.xpn` programs to SFZ format for other samplers |
| `xpn_xometa_dna_bulk_export.py` | Bulk export DNA vectors to CSV/JSON for external analysis |
| `xpn_xometa_schema_version_migrator.py` | Migrate old `.xometa` files to current schema version |
| `xpn_xometa_to_pack_bridge.py` | Bridge `.xometa` metadata into pack manifest fields |
| `xpn_params_sidecar_spec.py` | Specify and generate parameter sidecar files |
| `xpn_session_handoff.py` | Generate session handoff notes for async collaboration |
| `xpn_session_summary_generator.py` | Summarize a development session's preset output |

**Generative kit tools** (creative/experimental):

| Tool | Purpose |
|---|---|
| `xpn_attractor_kit.py` | Strange attractor rhythm patterns as kit programs |
| `xpn_ca_presets_kit.py` | Cellular automata–derived kit configurations |
| `xpn_entropy_kit.py` | Entropy/noise-seeded kit generation |
| `xpn_gene_kit.py` | Genetic algorithm preset breeding |
| `xpn_gravitational_wave_kit.py` | Gravitational wave–inspired rhythm kit |
| `xpn_lsystem_kit.py` | L-system fractal rhythm kit |
| `xpn_pendulum_kit.py` | Pendulum physics rhythm kit |
| `xpn_seismograph_kit.py` | Seismographic data–mapped kit |
| `xpn_transit_kit.py` | Transit timetable–mapped rhythm kit |
| `xpn_turbulence_kit.py` | Turbulence mathematics–inspired kit |
| `xpn_climate_kit.py` | Climate data–mapped generative kit |
| `xpn_braille_rhythm_kit.py` | Braille pattern–mapped rhythm kit |
| `xpn_poetry_kit.py` | Poetry meter–derived rhythm kit |
| `xpn_curiosity_engine.py` | Exploratory random-walk preset generator |
| `xpn_evolution_builder.py` | Evolutionary sequence of presets (parent → child → grandchild) |
| `xpn_monster_rancher.py` | "Monster Rancher" style preset breeding from two parents |
| `xpn_deconstruction_builder.py` | Systematically deconstruct a preset to isolate each component |
| `xpn_articulation_builder.py` | Build articulation sets (legato/staccato/pizz etc.) |
| `xpn_stem_pack_designer.py` | Design stem pack structure for a complex preset |

---

## 4. Running Tools

### Prerequisites

```sh
# All tools are stdlib only — no pip install needed
# Run from the repo root
cd ~/Documents/GitHub/XO_OX-XOceanus
```

### Common Invocations

```sh
# Check fleet health (always start here)
python3 Tools/xpn_fleet_health_dashboard.py

# Find engine with worst coverage
python3 Tools/xpn_engine_preset_gap_reporter.py --engine OPAL --preset-dir Presets/XOceanus

# Close coupling coverage gaps automatically
python3 Tools/xpn_coverage_final_sprint.py

# Run DNA diversity diagnosis
python3 Tools/xpn_fleet_dna_diversity_report.py

# Inject extreme-zone presets to improve diversity score
python3 Tools/xpn_preset_dna_extreme_filler.py

# Validate all .xometa files
python3 Tools/xpn_xometa_batch_validator.py Presets/XOceanus --strict

# Full XPN export for ONSET drum kit
python3 Tools/oxport.py run --engine Onset --wavs-dir /path/to/wavs --output-dir /tmp/onset_pack

# Generate TIDE TABLES spec
python3 Tools/xpn_tide_tables_pack_builder.py

# Run full QA on a finished pack
python3 Tools/xpn_full_qa_runner.py --xpn /path/to/MyPack.xpn
```

### Snapshot Outputs

Analytics tools write JSON snapshots to `Docs/snapshots/`:

| File | Written by |
|---|---|
| `fleet_health_latest.json` | `xpn_fleet_health_dashboard.py` |
| `diversity_score_latest.json` | `xpn_fleet_diversity_score_v2.py` |
| `dna_diversity_analysis.json` | `xpn_dna_diversity_analyzer.py` |
| `coupling_coverage_latest.json` | `xpn_coupling_coverage_tracker_v2.py` |
| `fleet_YYYYMMDD.json` | `xpn_preset_fleet_snapshot.py` |

---

## 5. Key Fleet Metrics (2026-03-16)

| Metric | Value |
|---|---|
| Total presets | 5,573 |
| Moods | 7 (Foundation / Atmosphere / Entangled / Prism / Flux / Aether / Family) |
| Engines registered | 34 |
| Coupling pairs covered | 561 / 561 (100%) |
| Entangled presets | 2,576 |
| Fleet diversity score | 0.1535 (target ≥0.35) |
| Brightness compression | 60.6% midrange (primary bottleneck) |
| Warmth compression | 55.4% midrange |
| Health rating | CRITICAL (diversity) |

The coupling coverage is complete. The primary remaining work is **DNA diversity** — pushing the fleet diversity score above 0.35 by injecting extreme-zone presets, especially in the brightness dimension.

**Priority tools for diversity work:**
1. `xpn_fleet_health_dashboard.py` — diagnose current state
2. `xpn_preset_dna_extreme_filler.py` — smart extreme-zone injector
3. `xpn_extreme_brightness_v2_pack.py` — brightness-specific fills
4. `xpn_dna_gap_finder.py` — find specific underrepresented regions
5. `xpn_fleet_diversity_score_v2.py` — verify improvement after fills

---

## 6. Non-xpn_ Supporting Scripts

These live in `Tools/` alongside the xpn_ suite and handle lower-level operations:

| Tool | Purpose |
|---|---|
| `oxport.py` | Master XPN export pipeline orchestrator (see §3.5) |
| `validate_presets.py` | Comprehensive `.xometa` validator (schema + DNA + naming) |
| `compute_preset_dna.py` | Compute 6D DNA vectors from parameter values |
| `audit_sonic_dna.py` | Per-engine DNA coverage audit |
| `add_missing_dna.py` | Add missing DNA blocks in batch |
| `find_missing_dna.py` | List presets without DNA blocks |
| `breed_presets.py` | Genetic crossover preset breeding |
| `rename_weak_presets.py` | Rename presets with poor names |
| `migrate_xocmeta_to_xometa.py` | Schema migration: `.xocmeta` → `.xometa` format |
| `apply_renames.py` | Apply a rename map to preset files |
| `extract_cpp_presets.py` | Extract preset definitions from C++ source files |
| `generate_preset_params.py` | Generate parameter blocks for new presets |
| `fix_bob_presets.py` | Fix OBLONG (BOB) preset-specific issues |
| `fix_drift_presets.py` | Fix ODYSSEY (drift) preset-specific issues |
| `fix_dub_presets.py` | Fix OVERDUB preset-specific issues |
| `fix_overworld_presets.py` | Fix OVERWORLD preset-specific issues |
| `fix_xobese_xpms.py` | Fix OBESE XPM files |
| `generate_*_presets.py` | Per-engine preset batch generators (legacy, superseded by xpn_ tools) |
| `site_audio_export.py` | Export audio clips for the XO-OX.org website |
