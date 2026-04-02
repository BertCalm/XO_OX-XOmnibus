# XO_OX XPN Toolchain Catalog
## Python Tools in Tools/ & Tools/_archive/

Generated: 2026-03-29

---

## CORE PIPELINE ORCHESTRATORS

These tools are the heart of the export pipeline and orchestrate multiple stages.

### oxport.py (2774 lines)
**Category**: RENDER, EXPORT, PACK
**Description**: Oxport — XO_OX Expansion Pack Pipeline Orchestrator. Chains the full XPN export pipeline into a single command.
**Pipeline Stages**:
  1. render_spec       — Generate render specifications from .xometa presets
  2. categorize        — Classify WAV samples into voice categories
  3. expand            — Expand flat kits into velocity/cycle/smart WAV sets
  4. qa                — Perceptual QA check on rendered WAV files
  5. smart_trim        — Auto-trim silence tails and add fade-out on rendered WAVs
  6. export            — Generate .xpm programs (drum or keygroup, per engine)
  7. cover_art         — Generate branded procedural cover art
  8. complement_chain  — Generate primary+complement XPM variant pairs (Artwork collection)
  9. preview           — Generate 15-second preview audio for each program
 10. package           — Package everything into a .xpn archive
**Usage**: `python3 oxport.py run --engine Onset --output-dir /path/to/out`
**Imports**: 25 other tools in the pipeline
**Status**: ACTIVE (core orchestrator)

### xpn_bundle_builder.py (1436 lines)
**Category**: PACK, EXPORT
**Description**: Flexible multi-engine expansion pack builder with 3 bundling modes, plus Collection mode for hierarchical multi-quad/set XPN packaging.
**Key Features**:
  - Multiple bundle modes (standard, flat, lite)
  - Collection mode for Kitchen Collection style hierarchical packaging
  - Per-program metadata inheritance
**Imports**: xpn_cover_art, xpn_drum_export, xpn_keygroup_export, xpn_packager, xpn_params_sidecar_spec
**Status**: ACTIVE

### xpn_batch_export.py (371 lines)
**Category**: EXPORT
**Description**: Batch XPN Export Orchestrator. Runs the full oxport.py pipeline for multiple engines in sequence (or in parallel) from a JSON config.
**Usage**: `python3 xpn_batch_export.py --config batch.json --parallel 4`
**Imports**: oxport
**Status**: ACTIVE

---

## RENDER & AUDIO GENERATION

Tools for WAV rendering and audio conversion.

### oxport_render.py (386 lines)
**Category**: RENDER
**Description**: Fleet Render Automation. Automates WAV rendering from XOceanus by sending MIDI to the plugin and recording audio output via loopback (BlackHole on macOS).
**Key Features**:
  - MIDI-based rendering to audio interface
  - BlackHole loopback support
  - Realtime progress tracking
**Usage**: `python3 oxport_render.py --engine Onset --output-dir /path/to/out --midi-device "BlackHole"`
**Called By**: oxport
**Status**: ACTIVE

### xpn_drum_export.py (377 lines)
**Category**: EXPORT, RENDER
**Description**: Drums-specific XPM program generation from WAV + metadata.
**Functionality**:
  - One-shot drum sample mapping
  - Velocity zone assignment
  - Drum kit layout creation
**Imports**: xpn_cover_art
**Called By**: oxport, xpn_bundle_builder, xpn_kit_expander, xpn_keygroup_export
**Status**: ACTIVE

### xpn_keygroup_export.py (408 lines)
**Category**: EXPORT, RENDER
**Description**: Keygroup-based XPM program generation for pitched instruments and sampled melodic content.
**Functionality**:
  - Zone-based pitch mapping
  - Root note detection integration
  - Multi-note sample layout
**Imports**: xpn_drum_export
**Called By**: oxport, xpn_articulation_builder, xpn_bundle_builder
**Status**: ACTIVE

### xpn_render_spec.py (611 lines)
**Category**: RENDER
**Description**: XPN Render Spec Generator. Reads .xometa preset files and generates render specifications that tell the producer exactly what WAV files to record.
**Core Function**: Bridges .xometa presets (live synthesis) to WAV render jobs
**Usage**: `python3 xpn_render_spec.py --engine Onset --output-dir /path/to/out`
**Called By**: oxport
**Status**: ACTIVE (duplicate: also xpn_render_spec_generator.py)

### xpn_render_spec_generator.py (452 lines)
**Category**: RENDER
**Description**: XPN Render Spec Generator (variant). Alternative implementation of render spec generation.
**Status**: ACTIVE (see xpn_render_spec.py)

---

## SAMPLE PROCESSING & CATEGORIZATION

Tools for analyzing, categorizing, and processing audio samples.

### xpn_sample_categorizer.py (383 lines)
**Category**: CATEGORIZE
**Description**: Classify WAV samples into voice categories (drums, bass, pads, etc.) using audio analysis heuristics.
**Analysis Methods**:
  - Spectral centroid
  - Attack time
  - RMS energy
  - Zero-crossing rate
**Called By**: oxport
**Status**: ACTIVE

### xpn_classify_instrument.py (547 lines)
**Category**: CATEGORIZE
**Description**: Instrument classifier using audio fingerprinting (spectral analysis, pitch content, energy envelope).
**Outputs**:
  - Instrument type prediction
  - Confidence score
  - Suggested Pad or Keygroup zone assignment
**Imports**: xpn_classify_instrument (self-reference, likely for module initialization)
**Called By**: oxport, xpn_adaptive_velocity, xpn_auto_dna, xpn_choke_group_assigner, xpn_kit_completeness, xpn_normalize
**Status**: ACTIVE

### xpn_smart_trim.py (397 lines)
**Category**: TRIM
**Description**: Auto-trim silence tails and add fade-out to rendered WAVs with configurable thresholds.
**Parameters**:
  - Silence threshold (dB)
  - Fade-out duration (ms)
  - Attack detection
**Called By**: oxport
**Status**: ACTIVE

### xpn_sample_audit.py (312 lines)
**Category**: QA
**Description**: Audit rendered WAV files for quality metrics (bit depth, sample rate, clipping, dynamic range).
**Checks**:
  - Sample rate consistency
  - Bit depth validation
  - Peak level monitoring
  - Silence detection
**Status**: ACTIVE (indirectly via oxport)

### xpn_sample_fingerprinter.py (283 lines)
**Category**: DIVERSITY
**Description**: Audio fingerprinting engine for detecting duplicate or near-identical samples via spectral hashing.
**Outputs**:
  - MD5-like fingerprint string
  - Similarity matrix
  - Dedup recommendations
**Status**: SUPPLEMENTARY (QA/analysis)

### xpn_sample_metadata_embedder.py (294 lines)
**Category**: METADATA
**Description**: Embed metadata (engine, preset, key, intensity) into WAV files as ID3/RIFF tags.
**Status**: SUPPLEMENTARY

### xpn_sample_loop_checker.py (256 lines)
**Category**: QA
**Description**: Detect loop points in WAV files and validate loop correctness (no clicks at boundaries).
**Status**: SUPPLEMENTARY (for looping drum kits)

---

## SOUND DESIGN & DNA (Sonic Diversity)

Tools for preset quality, diversity analysis, and DNA assignment.

### compute_preset_dna.py (1101 lines)
**Category**: DIVERSITY
**Description**: Calculate 6D Sonic DNA vectors for presets (brightness, warmth, movement, density, space, aggression).
**Algorithms**:
  - Parameter-to-DNA mapping
  - Feature extraction from .xometa files
  - Dimensionality reduction validation
**Called By**: breed_presets
**Status**: ACTIVE

### xpn_auto_dna.py (323 lines)
**Category**: DIVERSITY
**Description**: Automatically assign DNA vectors to presets using heuristic analysis of parameter settings.
**Method**: Parameterizes FILTER, MODULATION, AMPLITUDE, and TIMBRE dimensions
**Imports**: xpn_classify_instrument
**Called By**: oxport, xpn_adaptive_velocity
**Status**: ACTIVE

### xpn_dna_interpolator.py (407 lines)
**Category**: DIVERSITY
**Description**: Interpolate DNA vectors between two presets to fill coverage gaps in the 6D space.
**Usage**: Blend brightness/warmth/movement across preset libraries
**Status**: SUPPLEMENTARY (pre-export diversity analysis)

### xpn_dna_diversity_analyzer.py (389 lines)
**Category**: DIVERSITY
**Description**: Fleet-wide DNA diversity metrics (spread, clustering, coverage).
**Outputs**:
  - Diversity score (0–1)
  - Coverage heatmap
  - Gap identification
**Status**: SUPPLEMENTARY (QA analysis)

### xpn_dna_gap_finder.py (301 lines)
**Category**: DIVERSITY
**Description**: Identify underrepresented regions in 6D DNA space and recommend preset generation targets.
**Status**: SUPPLEMENTARY

### xpn_diversity_threshold_crosser.py (325 lines)
**Category**: DIVERSITY
**Description**: Enforce diversity thresholds (minimum distance between presets in DNA space).
**Status**: SUPPLEMENTARY

### xpn_fleet_dna_diversity_report.py (411 lines)
**Category**: DIVERSITY, QA
**Description**: Comprehensive fleet DNA diversity report with per-mood and per-engine breakdown.
**Status**: SUPPLEMENTARY (analytics)

### audit_sonic_dna.py (283 lines)
**Category**: DIVERSITY, QA
**Description**: Audit DNA assignments for consistency and correctness.
**Status**: SUPPLEMENTARY

### add_missing_dna.py (459 lines)
**Category**: DIVERSITY
**Description**: Backfill missing DNA vectors on presets using heuristic defaults.
**Status**: ARCHIVE-LIKE (one-off fix)

### find_missing_dna.py (144 lines)
**Category**: DIVERSITY
**Description**: Identify presets lacking DNA assignments.
**Status**: ARCHIVE-LIKE

### rename_dna_coded_presets.py (126 lines)
**Category**: DIVERSITY, NAMING
**Description**: Rename presets using DNA codes as naming scheme (e.g., "B5W2M3_DarkPad").
**Status**: ARCHIVE-LIKE (legacy naming scheme)

---

## COVER ART & VISUAL ASSETS

Tools for generating and managing preset/pack visual branding.

### xpn_cover_art.py (1107 lines)
**Category**: COVER_ART
**Description**: XPN Cover Art Generator. Generates branded procedural cover art for MPC expansion packs.
**Outputs**:
  - artwork_500.png   (500×500, standard MPC thumbnail)
  - artwork_1000.png  (1000×1000, hi-res)
  - artwork_2x.png    (2× DPI variant)
**Color Palette**: Animated gradients, bioluminescence, water column metaphor
**Called By**: oxport, xpn_bundle_builder, xpn_cover_art_batch, xpn_drum_export, xpn_spectral_fingerprint
**Status**: ACTIVE

### xpn_cover_art_generator_v2.py (482 lines)
**Category**: COVER_ART
**Description**: Enhanced cover art generator with improved procedural design.
**Called By**: oxport
**Status**: ACTIVE (newer variant)

### xpn_cover_art_batch.py (344 lines)
**Category**: COVER_ART
**Description**: Batch cover art generation for multiple presets/programs.
**Imports**: xpn_cover_art
**Status**: SUPPLEMENTARY

### xpn_cover_art_audit.py (267 lines)
**Category**: COVER_ART, QA
**Description**: Validate cover art resolution, color space, and metadata.
**Status**: SUPPLEMENTARY (QA)

### xpn_optic_fingerprint.py (289 lines)
**Category**: COVER_ART, DIVERSITY
**Description**: Generate unique fingerprint graphics based on Optic engine audio analysis (Lissajous waveforms, frequency analysis).
**Imports**: (used by xpn_cover_art)
**Called By**: xpn_cover_art
**Status**: SUPPLEMENTARY (visual identity)

### xpn_spectral_fingerprint.py (356 lines)
**Category**: COVER_ART, DIVERSITY
**Description**: Generate spectral analysis visualization (frequency domain, time-frequency heat maps).
**Imports**: xpn_cover_art, self
**Called By**: xpn_cover_art
**Status**: SUPPLEMENTARY

---

## VALIDATION & QA

Tools for comprehensive quality assurance and validation.

### xpn_pack_qa_report.py (650 lines)
**Category**: QA
**Description**: Top-of-stack Pack QA Report Generator. Runs all key validation checks against a pack directory or .xpn ZIP and aggregates findings into a single master report.
**Checks**:
  - XPM structural validation
  - Preset DNA completeness
  - Sample audio quality
  - Naming consistency
  - Preset count and distribution
**Output Format**: JSON + human-readable markdown
**Called By**: oxport
**Status**: ACTIVE

### validate_presets.py (620 lines)
**Category**: PRESET, QA
**Description**: Validate .xometa preset files for schema correctness, DNA completeness, naming uniqueness, and parameter validity.
**Validation Layers**:
  - Schema (JSON structure)
  - Data types and ranges
  - Required fields
  - Naming convention (2–3 words, < 30 chars, unique)
  - DNA vector validity (6D bounds)
**Called By**: oxport
**Status**: ACTIVE

### xpn_validator.py (408 lines)
**Category**: QA
**Description**: XPN package validator (checks .xpn ZIP structure, manifest, program counts).
**Called By**: oxport
**Status**: ACTIVE

### xpn_xpm_validator_strict.py (487 lines)
**Category**: QA
**Description**: Strict XPM program file validator with per-layer, per-zone, and per-articulation checks.
**Validation Scope**:
  - Layer count and configuration
  - Zone root note assignments
  - Velocity range coverage
  - Sample path validity
  - Metadata completeness
**Status**: SUPPLEMENTARY (deep audit)

### xpn_qa_checker.py (356 lines)
**Category**: QA
**Description**: Quick QA checks on rendered WAVs and export output.
**Checks**:
  - File existence
  - Bit depth / sample rate
  - Peak level
  - Duration bounds
**Called By**: oxport
**Status**: ACTIVE

### xpn_full_qa_runner.py (382 lines)
**Category**: QA
**Description**: Orchestrate all QA checks in sequence with parallel execution support.
**Status**: SUPPLEMENTARY (orchestration wrapper)

### xpn_pack_integrity_check.py (293 lines)
**Category**: QA
**Description**: Deep integrity check on .xpn ZIP (file hashes, manifest consistency, missing dependencies).
**Status**: SUPPLEMENTARY

### xpn_community_qa.py (708 lines)
**Category**: QA
**Description**: Community-friendly QA report with emphasis on usability, playability, and preset variety.
**Audience**: Non-technical users
**Status**: SUPPLEMENTARY (user-facing)

### xpn_profile_validator.py (294 lines)
**Category**: QA
**Description**: Validate MPC user profiles and project configurations for compatibility.
**Status**: SUPPLEMENTARY

### xpn_profile_preset_selector.py (267 lines)
**Category**: QA
**Description**: Recommend presets based on MPC user playing style and skill level.
**Imports**: xpn_qa_decision_log
**Status**: SUPPLEMENTARY

### xpn_preset_duplicate_detector.py (234 lines)
**Category**: PRESET, QA
**Description**: Detect duplicate or near-identical presets using DNA distance and parameter similarity.
**Status**: SUPPLEMENTARY

---

## PRESET GENERATION & VARIATION

Tools for creating, expanding, and diversifying preset libraries.

### breed_presets.py (422 lines)
**Category**: PRESET
**Description**: Genetic algorithm-style preset breeding: crossover two parent presets to generate offspring.
**Algorithm**:
  - DNA recombination
  - Parameter interpolation
  - Mutation with configurable rates
**Imports**: compute_preset_dna
**Status**: SUPPLEMENTARY (preset expansion)

### xpn_preset_name_generator.py (287 lines)
**Category**: NAMING, PRESET
**Description**: Generate evocative, rule-compliant preset names (2–3 words, < 30 chars, unique).
**Algorithm**: Template-based generation + dictionary lookup
**Imports**: xpn_pack_naming_validator
**Called By**: xpn_coupling_recipe_expander
**Status**: SUPPLEMENTARY

### xpn_preset_tag_recommender.py (345 lines)
**Category**: PRESET, NAMING
**Description**: Recommend semantic tags for presets based on parameter analysis and audio characteristics.
**Tags**: mood, timbre, technique (e.g., "pad", "bright", "evolving")
**Status**: SUPPLEMENTARY

### xpn_intent_generator.py (298 lines)
**Category**: PRESET
**Description**: Generate preset intent/character description based on parameter settings and DNA.
**Output**: Human-readable phrase describing the preset's sonic intent
**Called By**: oxport
**Status**: SUPPLEMENTARY

### xpn_preset_variation_generator.py (267 lines)
**Category**: PRESET, DIVERSITY
**Description**: Generate sonic variations of a preset by perturbing parameters within musically-meaningful ranges.
**Status**: SUPPLEMENTARY

### xpn_preset_mood_classifier.py (312 lines)
**Category**: PRESET, DIVERSITY
**Description**: Classify presets into mood categories (Foundation, Atmosphere, Entangled, etc.) using parameter analysis.
**Status**: SUPPLEMENTARY

### xpn_preset_crossfade_designer.py (394 lines)
**Category**: PRESET
**Description**: Design smooth crossfade envelopes between parameter sets for variation/morph presets.
**Status**: SUPPLEMENTARY

### xpn_preset_aging_simulator.py (267 lines)
**Category**: PRESET
**Description**: Simulate parameter drift and "aging" of a preset over time (vintage modeling concept).
**Status**: SUPPLEMENTARY

### xpn_preset_dna_extreme_filler.py (289 lines)
**Category**: DIVERSITY, PRESET
**Description**: Generate "extreme" presets at the boundaries of DNA space (maximum brightness, minimum warmth, etc.).
**Purpose**: Ensure full exploration of synth capability
**Status**: SUPPLEMENTARY

### generate_broth_presets.py (1877 lines)
**Category**: PRESET
**Description**: Generate complete preset library for Broth quad (OVERWASH, OVERWORN, OVERFLOW, OVERCAST).
**Status**: ARCHIVE (specialized generator)

### generate_presets_overlap_outwit.py (1238 lines)
**Category**: PRESET
**Description**: Generate presets for OVERLAP and OUTWIT engines.
**Status**: ARCHIVE (specialized generator)

### generate_submerged_presets.py (502 lines)
**Category**: PRESET
**Description**: Generate Submerged mood presets.
**Status**: ARCHIVE (specialized generator)

---

## PACKAGING & BUNDLING

Tools for assembling packs into final .xpn distributions.

### xpn_packager.py (478 lines)
**Category**: PACK, EXPORT
**Description**: Create final .xpn ZIP archives from exported programs, metadata, and assets.
**Functionality**:
  - ZIP compression
  - Manifest generation
  - License/credit embedding
  - Version tagging
**Imports**: xpn_coupling_recipes
**Called By**: oxport, xpn_bundle_builder, xpn_coupling_recipes
**Status**: ACTIVE

### xpn_pack_license_generator.py (247 lines)
**Category**: PACK, METADATA
**Description**: Generate license/attribution text for XPN packs (Creative Commons, samples, engine credits).
**Called By**: oxport (indirectly via xpn_packager)
**Status**: SUPPLEMENTARY

### xpn_pack_readme_generator.py (256 lines)
**Category**: PACK, METADATA
**Description**: Generate README.md documentation for XPN packs.
**Imports**: xpn_pack_readme_generator (self-reference)
**Status**: SUPPLEMENTARY

### xpn_pack_bundle_sizer.py (267 lines)
**Category**: PACK
**Description**: Estimate and optimize final .xpn bundle size (compression ratios, asset optimization).
**Status**: SUPPLEMENTARY (analytics)

### xpn_pack_version_bumper.py (145 lines)
**Category**: PACK, METADATA
**Description**: Increment version numbers in pack manifests.
**Status**: SUPPLEMENTARY (release management)

### xpn_pack_ci_config_generator.py (287 lines)
**Category**: PACK, METADATA
**Description**: Generate CI/CD configuration (GitHub Actions, etc.) for automated pack builds.
**Imports**: self-reference
**Status**: SUPPLEMENTARY

### xpn_collection_arc_validator.py (465 lines)
**Category**: PACK, QA
**Description**: Validate hierarchical collection structure (Kitchen Collection quads, nested engines, licensing).
**Status**: SUPPLEMENTARY

### xpn_collection_sequencer.py (414 lines)
**Category**: PACK
**Description**: Sequence engines/quads within a collection for optimal presentation order and progression.
**Status**: SUPPLEMENTARY

---

## EXPORT & TRANSLATION

Tools for converting between formats and generating export-specific metadata.

### xpn_expansion_json_builder.py (267 lines)
**Category**: EXPORT, METADATA
**Description**: Generate expansion.json metadata file for XPN packages.
**Contents**: Engine list, preset index, feature matrix, version info
**Status**: SUPPLEMENTARY

### xpn_manifest_generator.py (256 lines)
**Category**: METADATA, EXPORT
**Description**: Generate pack manifests (engine list, preset counts, file hashes, dependencies).
**Imports**: self-reference
**Called By**: oxport
**Status**: ACTIVE

### xpn_xometa_export_pipeline.py (378 lines)
**Category**: EXPORT
**Description**: Convert .xometa presets to other formats (JSON, CSV, etc.) for documentation or import.
**Status**: SUPPLEMENTARY

### xpn_xpm_batch_builder.py (312 lines)
**Category**: EXPORT
**Description**: Batch build .xpm program files from WAVs + metadata.
**Status**: SUPPLEMENTARY

### xpn_to_sfz.py (289 lines)
**Category**: EXPORT
**Description**: Convert XPM programs to SFZ sample format (for non-MPC DAWs).
**Status**: SUPPLEMENTARY (cross-format support)

### xpn_drum_export.py (see RENDER section)
**Status**: ACTIVE

### xpn_keygroup_export.py (see RENDER section)
**Status**: ACTIVE

### site_audio_export.py (254 lines)
**Category**: EXPORT
**Description**: Export preset audio for web/documentation (MP3/AAC formats).
**Purpose**: Generate preview audio for website/marketing
**Status**: SUPPLEMENTARY

---

## UTILITY & HELPERS

Smaller tools for specific tasks, often one-off or supporting other tools.

### xpn_kit_expander.py (289 lines)
**Category**: PACK, UTILITY
**Description**: Expand flat drum kits into velocity/cycle variants.
**Imports**: xpn_drum_export
**Called By**: oxport
**Status**: ACTIVE

### xpn_choke_group_assigner.py (404 lines)
**Category**: UTILITY
**Description**: Automatically assign choke group IDs to drum samples (hi-hat closure, etc.).
**Imports**: xpn_classify_instrument
**Called By**: oxport
**Status**: ACTIVE

### xpn_adaptive_velocity.py (413 lines)
**Category**: UTILITY
**Description**: Dynamically adjust velocity response curves based on preset characteristics.
**Imports**: xpn_auto_dna, xpn_classify_instrument
**Called By**: oxport
**Status**: ACTIVE

### xpn_auto_root_detect.py (315 lines)
**Category**: UTILITY
**Description**: Automatically detect root note of audio samples via pitch detection.
**Imports**: self-reference
**Status**: ACTIVE (supporting keygroup export)

### xpn_mpce_quad_builder.py (267 lines)
**Category**: PACK, UTILITY
**Description**: Build Kitchen Collection quads (4-engine sets) for MPCE platform.
**Called By**: oxport
**Status**: SUPPLEMENTARY

### xpn_tuning_systems.py (198 lines)
**Category**: UTILITY
**Description**: Support for alternative tuning systems (12-TET, Pythagorean, just intonation, etc.).
**Called By**: oxport
**Status**: SUPPLEMENTARY

### xpn_loudness_ledger.py (156 lines)
**Category**: UTILITY, QA
**Description**: Track and normalize loudness across preset library (LUFS, RMS, peak).
**Called By**: oxport
**Status**: SUPPLEMENTARY

### xpn_kit_completeness.py (234 lines)
**Category**: UTILITY, QA
**Description**: Check that drum kits have complete coverage of typical percussion zones.
**Imports**: xpn_classify_instrument
**Status**: SUPPLEMENTARY

### xpn_kit_validator.py (267 lines)
**Category**: UTILITY, QA
**Description**: Validate drum kit layer structure (zones, velocities, articulations).
**Status**: SUPPLEMENTARY

### xpn_kit_layering_advisor.py (298 lines)
**Category**: UTILITY
**Description**: Recommend layer structures for drum kits based on sample characteristics.
**Status**: SUPPLEMENTARY

### xpn_normalize.py (187 lines)
**Category**: UTILITY, NAMING
**Description**: Normalize preset/program metadata (whitespace, encoding, field formats).
**Imports**: xpn_classify_instrument
**Status**: SUPPLEMENTARY

### xpn_coupling_recipe_expander.py (234 lines)
**Category**: UTILITY, PRESET
**Description**: Expand coupling recipe templates into full preset specifications.
**Imports**: xpn_preset_name_generator
**Status**: SUPPLEMENTARY

### xpn_coupling_recipes.py (198 lines)
**Category**: UTILITY, PRESET
**Description**: Template library of popular cross-engine coupling configurations.
**Imports**: self-reference, xpn_packager
**Status**: SUPPLEMENTARY

### xpn_params_sidecar_spec.py (145 lines)
**Category**: METADATA, UTILITY
**Description**: Generate sidecar .json files with parameter documentation.
**Imported By**: xpn_bundle_builder
**Status**: SUPPLEMENTARY

### xpn_qa_decision_log.py (178 lines)
**Category**: QA, UTILITY
**Description**: Log and track QA decisions/overrides throughout the pipeline.
**Imports**: self-reference
**Called By**: xpn_profile_preset_selector, xpn_profile_validator
**Status**: SUPPLEMENTARY

---

## ANALYSIS & REPORTING

Tools for comprehensive fleet/pack analytics and health reporting.

### xpn_fleet_health_dashboard.py (345 lines)
**Category**: QA, ANALYSIS
**Description**: Comprehensive fleet health dashboard (average DNA coverage, per-engine diversity, QA status).
**Status**: SUPPLEMENTARY (analytics)

### xpn_fleet_health_summary.py (267 lines)
**Category**: QA, ANALYSIS
**Description**: High-level fleet health summary (total presets, average quality score, mood distribution).
**Status**: SUPPLEMENTARY

### xpn_fleet_diversity_score_v2.py (289 lines)
**Category**: DIVERSITY, ANALYSIS
**Description**: Calculate fleet-wide DNA diversity metrics (spatial coverage, clustering).
**Status**: SUPPLEMENTARY

### xpn_engine_preset_gap_reporter.py (312 lines)
**Category**: ANALYSIS, PRESET
**Description**: Identify per-engine preset gaps (missing moods, underrepresented DNA regions).
**Called By**: oxport (indirectly)
**Status**: SUPPLEMENTARY

### xpn_engine_coverage_mapper.py (289 lines)
**Category**: ANALYSIS, DIVERSITY
**Description**: Visual heatmap of preset coverage across all engines × moods × DNA dimensions.
**Status**: SUPPLEMENTARY

### xpn_expansion_bundle_profiler.py (278 lines)
**Category**: ANALYSIS, PACK
**Description**: Profile .xpn bundles for structure, size, and consistency.
**Status**: SUPPLEMENTARY

### xpn_pack_health_dashboard.py (334 lines)
**Category**: ANALYSIS, PACK, QA
**Description**: Per-pack health metrics (QA score, DNA coverage, preset counts, version info).
**Status**: SUPPLEMENTARY

### xpn_session_summary_generator.py (267 lines)
**Category**: ANALYSIS, UTILITY
**Description**: Generate session handoff summary (what was built, what QA found, next steps).
**Status**: SUPPLEMENTARY

### xpn_pack_diff.py (267 lines)
**Category**: ANALYSIS, PACK
**Description**: Compare two XPN pack versions (file diffs, metadata changes).
**Status**: SUPPLEMENTARY

---

## DEAD CODE (Not Imported By Pipeline)

The following tools are likely one-off utilities, legacy code, or specialized generators that are not active in the standard oxport pipeline:

### Large Legacy Generators
- **generate_onset_presets.py** (2291 lines) — Specialized ONSET preset generator
- **xoutshine.py** (2049 lines) — Sample instrument forge (separate product)
- **generate_broth_presets.py** (1877 lines) — Kitchen Collection Broth quad presets

### Preset Fixup Scripts (Archived)
- **add_missing_dna.py** (459 lines) — One-off DNA backfill
- **fix_f4_audit.py** (504 lines) — Specific F4 audit fixes
- **fix_mood_and_entangled.py** (320 lines) — Legacy mood/entanglement fixes
- **rename_dna_coded_presets.py** (126 lines) — Legacy naming scheme
- **preset_migration_sprint1.py** (330 lines) — Migration from old preset format
- **xoceanus_preset_migration.py** (686 lines) — Migration to current format

### Specialized Generators (Archive)
- **generate_presets_overlap_outwit.py** (1238 lines) — OVERLAP/OUTWIT specialized generator
- **generate_submerged_presets.py** (502 lines) — Submerged mood specialized
- **generate_outwit_blocks23.py** (927 lines) — OUTWIT block-level content
- **generate_preset_params.py** (656 lines) — Parameter-based preset generation
- **extract_cpp_presets.py** (652 lines) — Legacy C++ preset extraction

### Single-Task Tools (Often Run Standalone)
- **extract_cpp_presets.py** (652 lines) — Extract presets from C++ source
- **quarantine_empty_presets.py** (177 lines) — Isolate empty/broken presets
- **rename_debug_presets.py** (476 lines) — Strip debug prefixes
- **dedup_preset_names.py** (205 lines) — Find and fix duplicate names
- **breed_presets.py** (422 lines) — Genetic algorithm preset generation (not called by oxport)
- **find_missing_dna.py** (144 lines) — Find presets lacking DNA
- **audit_sonic_dna.py** (283 lines) — Audit DNA quality
- **preset_audit.py** (370 lines) — General preset audit
- **validate_presets.py** (620 lines) — Comprehensive preset validation (called by oxport but also standalone)

### Analysis & Reporting (Not in Pipeline)
- **xpn_pack_analytics.py**, **xpn_pack_ab_tester.py**, **xpn_pack_compare_report.py** — Post-release analytics
- **xpn_preset_similarity_matrix.py**, **xpn_preset_clustering_report.py** — Preset analysis
- **xpn_preset_search_engine.py**, **xpn_preset_browser.py** — User-facing search tools
- **xpn_pack_discovery_optimizer.py** — Marketing/discovery optimization

### Metadata Generators (Not in Pipeline)
- **xpn_pack_marketing_copy.py** (256 lines) — Generate marketing descriptions
- **xpn_pack_readme_generator.py** (256 lines) — Generate README
- **xpn_pack_changelog_tracker.py** (245 lines) — Track changelog entries

---

## ARCHIVE DIRECTORY (_archive/)

165 files in Tools/_archive/, categorized by type:

### Legacy Preset Generators (Archive)
- generate_onset_presets.py (removed from core, now in archive)
- generate_opal_presets.py (1234 lines, archived)
- generate_organon_presets.py (1089 lines, archived)
- generate_organon_coupling_presets.py (834 lines, archived)
- generate_ouroboros_presets.py (756 lines, archived)
- generate_overbite_presets.py (823 lines, archived)
- generate_overworld_presets.py (934 lines, archived)
- generate_constellation_presets.py (430 lines, archived)
- generate_coupling_presets.py (233 lines, archived)
- ~60 other specialized preset generators

### Legacy Preset Fixups (Archive)
- fix_bob_presets.py, fix_drift_presets.py, fix_dub_presets.py, fix_overworld_presets.py — Engine-specific fixes
- fix_preset_schema.py (333 lines) — Schema migration
- rename_weak_presets.py — Rename low-quality presets
- cleanup_task{1-8}_*.py — Sequential cleanup pass scripts

### Format Conversions (Archive)
- xpn_aether_*.py (4 variants) — Format converters (likely obsolete)
- Various format/schema migration scripts

---

## DEPENDENCY MAP (Simplified)

### Core Pipeline Call Chain
```
oxport.py (ORCHESTRATOR)
├── oxport_render.py (RENDER WAVs)
├── xpn_sample_categorizer.py (CATEGORIZE)
├── xpn_kit_expander.py (EXPAND)
│   └── xpn_drum_export.py
├── xpn_smart_trim.py (TRIM)
├── xpn_drum_export.py (EXPORT — drums)
│   └── xpn_cover_art.py
│       ├── xpn_optic_fingerprint.py
│       └── xpn_spectral_fingerprint.py
├── xpn_keygroup_export.py (EXPORT — pitched)
│   ├── xpn_drum_export.py
│   └── xpn_articulation_builder.py
├── xpn_cover_art.py (COVER ART)
├── xpn_cover_art_generator_v2.py (ALT)
├── xpn_packager.py (PACKAGE)
│   └── xpn_coupling_recipes.py
├── xpn_bundle_builder.py (BUNDLE MODE)
│   ├── xpn_cover_art.py
│   ├── xpn_drum_export.py
│   ├── xpn_keygroup_export.py
│   ├── xpn_packager.py
│   └── xpn_params_sidecar_spec.py
├── validate_presets.py (VALIDATE)
├── xpn_validator.py
├── xpn_qa_checker.py
└── xpn_pack_qa_report.py
```

### Secondary Support Chain
```
xpn_classify_instrument.py (CATEGORIZATION HUB)
├── xpn_adaptive_velocity.py
├── xpn_auto_dna.py
├── xpn_choke_group_assigner.py
├── xpn_kit_completeness.py
└── xpn_normalize.py

compute_preset_dna.py (DNA COMPUTATION)
└── breed_presets.py (GENETIC BREEDING)

xpn_preset_name_generator.py (NAMING)
└── xpn_coupling_recipe_expander.py
    └── xpn_preset_name_generator.py (used by many)
```

---

## SUMMARY STATISTICS

| Category | Count | Key Tools | Status |
|----------|-------|-----------|--------|
| RENDER | 5 | oxport_render, xpn_drum_export, xpn_keygroup_export, xpn_render_spec* | ACTIVE |
| EXPORT | 8 | oxport, xpn_bundle_builder, xpn_batch_export, xpn_packager, xpn_to_sfz | ACTIVE |
| CATEGORIZE | 2 | xpn_classify_instrument, xpn_sample_categorizer | ACTIVE |
| TRIM | 1 | xpn_smart_trim | ACTIVE |
| QA | 12+ | xpn_pack_qa_report, validate_presets, xpn_validator, xpn_qa_checker | ACTIVE |
| COVER_ART | 5 | xpn_cover_art, xpn_cover_art_generator_v2, xpn_optic_fingerprint | ACTIVE |
| NAMING | 6 | xpn_preset_name_generator, xpn_preset_tag_recommender, xpn_normalize | SUPPLEMENTARY |
| DIVERSITY | 12 | compute_preset_dna, xpn_auto_dna, xpn_dna_interpolator, xpn_fleet_dna_diversity_report | SUPPLEMENTARY |
| PACK | 8 | xpn_bundle_builder, xpn_collection_sequencer, xpn_pack_license_generator | SUPPLEMENTARY |
| PRESET | 15+ | breed_presets, various generators, xpn_preset_variation_generator | MIXED |
| UTILITY | 10+ | xpn_kit_expander, xpn_choke_group_assigner, xpn_tuning_systems | SUPPLEMENTARY |
| METADATA | 6 | xpn_manifest_generator, xpn_expansion_json_builder, xpn_params_sidecar_spec | SUPPLEMENTARY |
| ANALYSIS | 10+ | xpn_fleet_health_dashboard, xpn_pack_health_dashboard | SUPPLEMENTARY |
| **TOTAL CORE** | 233 | — | — |
| **TOTAL ARCHIVE** | 165 | — | **LEGACY** |

---

## RECOMMENDATIONS FOR OXPORT PIPELINE

### Critical Path (Must Work)
1. **oxport.py** — orchestrator (2774 lines, all pipeline logic)
2. **oxport_render.py** — WAV rendering (386 lines)
3. **xpn_render_spec.py** — render spec generation (611 lines)
4. **xpn_sample_categorizer.py** — categorization (383 lines)
5. **xpn_classify_instrument.py** — classification hub (547 lines, used by 6 other tools)
6. **xpn_drum_export.py** — drum XPM generation (377 lines)
7. **xpn_keygroup_export.py** — pitched XPM generation (408 lines)
8. **xpn_cover_art.py** — cover art (1107 lines)
9. **xpn_packager.py** — ZIP assembly (478 lines)
10. **xpn_pack_qa_report.py** — QA aggregation (650 lines)

### High-Value Support
- **compute_preset_dna.py** (1101 lines) — DNA computation for diversity
- **xpn_bundle_builder.py** (1436 lines) — multi-engine bundling
- **xpn_xpm_validator_strict.py** (487 lines) — deep QA

### Candidates for Archival/Cleanup
- **generate_broth_presets.py** (1877 lines) — specialized, one-time use
- **generate_presets_overlap_outwit.py** (1238 lines) — specialized, one-time use
- **xoutshine.py** (2049 lines) — separate product, not XPN pipeline
- **50+ specialized preset generators** — archive or consolidate

---

**Generated**: 2026-03-29
**Toolchain Version**: XPN Expansion Pack Pipeline (MPCE Ready)

