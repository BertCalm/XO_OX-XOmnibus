# XPN Tool Suite — Comprehensive Session Summary

**Generated**: 2026-03-16  

**Purpose**: Orient any team member or new Claude session to the full XPN toolkit in under 10 minutes.


## Suite Stats

| Metric                         | Value      |
| ------------------------------ | ---------- |
| Total Python tools             | 141        |
| Total lines of tool code       | 79,674     |
| Total spec files (Docs/specs/) | 126        |
| Date generated                 | 2026-03-16 |


## Tool Categories

| Category         | Count | Examples                                                                                                       |
| ---------------- | ----- | -------------------------------------------------------------------------------------------------------------- |
| Core Pipeline    | 11    | oxport.py, xpn_bundle_builder.py, xpn_cover_art.py, xpn_drum_export.py …                                       |
| QA / Validation  | 18    | audit_sonic_dna.py, xpn_collection_arc_validator.py, xpn_community_qa.py, xpn_coupling_preset_auditor.py …     |
| Build / Export   | 15    | site_audio_export.py, xpn_articulation_builder.py, xpn_batch_export.py, xpn_complement_renderer.py …           |
| Generation       | 25    | breed_presets.py, generate_constellation_presets.py, generate_coupling_presets.py, generate_library_fills.py … |
| Editing / Fixing | 14    | add_missing_dna.py, apply_renames.py, fix_bob_presets.py, fix_drift_presets.py …                               |
| Analysis         | 8     | xpn_kit_theme_analyzer.py, xpn_optic_fingerprint.py, xpn_pack_analytics.py, xpn_pack_compare_report.py …       |
| Search / Browse  | 5     | find_missing_dna.py, xpn_auto_root_detect.py, xpn_classify_instrument.py, xpn_preset_browser.py …              |
| Utility / Misc   | 45    | compute_preset_dna.py, extract_cpp_presets.py, rename_weak_presets.py, validate_presets.py …                   |


## Tool Listing by Category

Each entry shows: **tool name**, purpose summary, and CLI usage pattern.


### Core Pipeline (11 tools)

| Tool                      | Purpose                                    | CLI Usage                                                                                            |
| ------------------------- | ------------------------------------------ | ---------------------------------------------------------------------------------------------------- |
| oxport.py                 | XO_OX Expansion Pack Pipeline Orchestrator | # Full pipeline for an engine                                                                        |
| xpn_bundle_builder.py     | XO_OX Designs                              | # Build from a bundle profile JSON                                                                   |
| xpn_cover_art.py          | XO_OX Designs                              | from xpn_cover_art import generate_cover                                                             |
| xpn_drum_export.py        | XO_OX Designs                              | # Export with velocity layers (default)                                                              |
| xpn_keygroup_export.py    | XO_OX Designs                              | python xpn_keygroup_export.py                                                                        |
| xpn_kit_expander.py       | XO_OX Designs                              | # Expand single WAV into velocity layers                                                             |
| xpn_manifest_generator.py | XO_OX XPN pack manifest generator          | python xpn_manifest_generator.py --pack-name "OBESE Rising" --engine OBESE       --mood Foundation - |
| xpn_packager.py           | XO_OX Designs                              | from xpn_packager import package_xpn, XPNMetadata                                                    |
| xpn_render_spec.py        | XO_OX Designs                              | # Generate render spec for one preset                                                                |
| xpn_sample_categorizer.py | XO_OX Designs                              | # Classify and print results                                                                         |
| xpn_validator.py          | XO_OX Designs                              | # Validate a single .xpn file                                                                        |


### QA / Validation (18 tools)

| Tool                            | Purpose                                                        | CLI Usage                                                                                            |
| ------------------------------- | -------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| audit_sonic_dna.py              | XOceanus Sonic DNA Coverage Auditor                            | python3 audit_sonic_dna.py [--verbose]                                                               |
| xpn_collection_arc_validator.py | XO_OX Collection Arc Validator                                 | python xpn_collection_arc_validator.py <collection_dir> [--format text|json] [--strict]              |
| xpn_community_qa.py             | Headless QA orchestrator for XO_OX community pack submissions. | python xpn_community_qa.py --submission pack.submission.zip [--discord-output] [--strict]            |
| xpn_coupling_preset_auditor.py  | Coupling quality auditor for .xometa preset files.             | python xpn_coupling_preset_auditor.py <presets_dir>                                                  |
| xpn_cover_art_audit.py          | XO_OX Cover Art Auditor                                        | python xpn_cover_art_audit.py --xpn pack.xpn [--strict] [--format text|json]                         |
| xpn_dedup_checker.py            | SHA-256 hash index for XO_OX sample deduplication.             | python xpn_dedup_checker.py --index known_hashes.json                                                |
| xpn_expansion_json_lint.py      | XO_OX Designs                                                  | python xpn_expansion_json_lint.py expansion.json [--format text|json] [--strict]                     |
| xpn_full_qa_runner.py           | XO_OX Master QA Orchestrator                                   | python xpn_full_qa_runner.py --xpn pack.xpn [--tools-dir ./Tools]         [--format text|json] [--st |
| xpn_kit_validator.py            | XO_OX Designs                                                  | # Validate a single .xpm file                                                                        |
| xpn_layer_balance_checker.py    | XO_OX Designs                                                  | python xpn_layer_balance_checker.py pack.xpn                                                         |
| xpn_macro_coverage_checker.py   | XO_OX Designs                                                  | python xpn_macro_coverage_checker.py <presets_dir> [options]                                         |
| xpn_manifest_validator.py       | XO_OX Expansion Pack Manifest Validator                        | python xpn_manifest_validator.py --xpn pack.xpn [--strict] [--format text|json]                      |
| xpn_pack_integrity_check.py     | XO_OX Designs                                                  | python xpn_pack_integrity_check.py path/to/pack.xpn                                                  |
| xpn_qa_checker.py               | Perceptual quality checker for rendered WAV files.             | python xpn_qa_checker.py samples/kick_vel_01.wav                                                     |
| xpn_sample_audit.py             | Audit WAV samples in a .xpn pack before release.               | python xpn_sample_audit.py --xpn pack.xpn [--strict] [--format text|json]                            |
| xpn_sample_loop_checker.py      | Check WAV loop point quality in a .xpn pack.                   | python xpn_sample_loop_checker.py <pack.xpn> [--format text|json] [--strict]                         |
| xpn_stems_checker.py            | XO_OX MPCe Quad-Corner Pack Readiness Validator                | python xpn_stems_checker.py --stems ./stems/ --output report.json                                    |
| xpn_tuning_coverage_checker.py  | XO_OX Designs                                                  | python xpn_tuning_coverage_checker.py --xpn pack.xpn                                                 |


### Build / Export (15 tools)

| Tool                           | Purpose                             | CLI Usage                                                         |
| ------------------------------ | ----------------------------------- | ----------------------------------------------------------------- |
| site_audio_export.py           | XO_OX Brand Site Audio Exporter     | 1. Record 2-3 second clips from each engine's standalone app      |
| xpn_articulation_builder.py    | XO_OX Designs                       | # Key-switch mode (low-octave notes C0+ select articulation)      |
| xpn_batch_export.py            | XO_OX Batch XPN Export Orchestrator | python xpn_batch_export.py --config batch.json                    |
| xpn_complement_renderer.py     | XO_OX Designs                       | # Single engine, discover presets in a directory                  |
| xpn_deconstruction_builder.py  | XO_OX Designs                       | python xpn_deconstruction_builder.py                              |
| xpn_evolution_builder.py       | XO_OX Designs                       | python xpn_evolution_builder.py \                                 |
| xpn_export_report_generator.py | XO_OX Designs                       | python xpn_export_report_generator.py --output-dir ./dist/Onset   |
| xpn_kit_builder_assistant.py   | XO_OX Designs                       | python xpn_kit_builder_assistant.py [--output kit_spec.json]      |
| xpn_mpce_quad_builder.py       | XO_OX Designs                       | python xpn_mpce_quad_builder.py \                                 |
| xpn_render_spec_generator.py   | XO_OX Designs                       | # Generate spec for all ONSET presets in Foundation mood          |
| xpn_sample_pool_builder.py     | XO_OX Sample Pool Builder           | python xpn_sample_pool_builder.py \                               |
| xpn_setlist_builder.py         | XO_OX Designs                       | python xpn_setlist_builder.py --packs-dir ./dist --set-length 8 \ |
| xpn_submission_packager.py     | XO_OX Designs                       | python xpn_submission_packager.py \                               |
| xpn_tutorial_builder.py        | XO_OX Designs                       | python xpn_tutorial_builder.py \                                  |
| xpn_xometa_dna_bulk_export.py  | XO_OX Designs                       | python xpn_xometa_dna_bulk_export.py <presets_dir> \              |


### Generation (25 tools)

| Tool                                 | Purpose                                                                   | CLI Usage                                                                                            |
| ------------------------------------ | ------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| breed_presets.py                     | Genetic Sound Design                                                      | python3 breed_presets.py "Parent A Name" "Parent B Name" [--offspring N] [--mutation-rate 0.15]      |
| generate_constellation_presets.py    | Generate 530 .xometa presets for the Family Constellation engines.        | python3 generate_constellation_presets.py                                                            |
| generate_coupling_presets.py         | Generate cross-engine coupling presets for XOceanus.                      | python generate_coupling_presets.py                                                                  |
| generate_library_fills.py            | XOceanus Library Gap Fill Generator                                       | python generate_library_fills.py                                                                     |
| generate_onset_presets.py            | Generate XOnset .xometa preset files for the XOceanus gallery.            | python generate_onset_presets.py                                                                     |
| generate_opal_presets.py             | Generate 100 XOpal factory presets for XOceanus.                          | python generate_opal_presets.py                                                                      |
| generate_organon_coupling_presets.py | Generate cross-engine coupling presets featuring Organon + other engines. | python generate_organon_coupling_presets.py                                                          |
| generate_organon_presets.py          | Generate 120 XOrganon factory presets for XOceanus.                       | python generate_organon_presets.py                                                                   |
| generate_ouroboros_presets.py        | Generate XOuroboros .xometa preset files for XOceanus.                    | python generate_ouroboros_presets.py                                                                 |
| generate_overbite_presets.py         | Generate XOverbite .xometa preset files for XOceanus.                     | python generate_overbite_presets.py                                                                  |
| generate_overworld_presets.py        | Generate XOverworld .xometa preset files for XOceanus.                    | python generate_overworld_presets.py                                                                 |
| generate_preset_params.py            | Generate DSP parameter values for all factory .xocmeta presets.           | python generate_preset_params.py                                                                     |
| xpn_changelog_generator.py           | XO_OX Designs                                                             | python xpn_changelog_generator.py --diff-mode --old v1.0.xpn --new v1.1.xpn                          |
| xpn_choke_group_designer.py          | XO_OX Choke Group Designer                                                | # Analyze current choke assignments                                                                  |
| xpn_coupling_docs_generator.py       | xpn_coupling_docs_generator.py                                            | python xpn_coupling_docs_generator.py --xometa preset.xometa                                         |
| xpn_cover_art_generator_v2.py        | XO_OX Tools                                                               | python xpn_cover_art_generator_v2.py <pack.xpn> [--output cover.png] [--size 400|800]                |
| xpn_pack_index_generator.py          | XO_OX Designs                                                             | python xpn_pack_index_generator.py --packs-dir ./dist                                                |
| xpn_pack_readme_generator.py         | xpn_pack_readme_generator.py                                              | python xpn_pack_readme_generator.py         --pack-name "ONSET Ritual Drums"         --engine ONSET  |
| xpn_pack_thumbnail_generator.py      | XO_OX Tools                                                               | python xpn_pack_thumbnail_generator.py <pack.xpn> [--output cover.png] [--size 400]                  |
| xpn_preset_variation_generator.py    | XO_OX Designs                                                             | python xpn_preset_variation_generator.py --xpm base.xpm \                                            |
| xpn_preview_generator.py             | XO_OX Designs                                                             | # Generate preview for a drum pack                                                                   |
| xpn_quick_preview_generator.py       | XO_OX Designs                                                             | python xpn_quick_preview_generator.py <pack.xpn>                                                     |
| xpn_session_summary_generator.py     | XPN Tool Suite Comprehensive Session Summary Generator                    | python xpn_session_summary_generator.py [--tools-dir ./Tools] [--specs-dir ./Docs/specs] [--output s |
| xpn_variation_generator.py           | XO_OX Designs                                                             | python xpn_variation_generator.py my_pack.xpn --type tone --direction felix --output ./variations/   |
| xpn_velocity_curve_designer.py       | XO_OX Designs                                                             | # Default Vibe curve, text output                                                                    |


### Editing / Fixing (14 tools)

| Tool                          | Purpose                                                                 | CLI Usage                                                                                            |
| ----------------------------- | ----------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| add_missing_dna.py            | Auto-Populate Missing Sonic DNA Blocks                                  | python3 Tools/add_missing_dna.py --dry-run    # preview only, no writes                              |
| apply_renames.py              | Bulk rename script for XOceanus preset files, documentation, and tools. | python apply_renames.py                                                                              |
| fix_bob_presets.py            | Migrate XOblong (Oblong/Bob) preset parameters                          | python3 Tools/fix_bob_presets.py [--dry-run]                                                         |
| fix_drift_presets.py          | Migrate XOdyssey (Odyssey/Drift) preset parameters                      | python3 Tools/fix_drift_presets.py [--dry-run]                                                       |
| fix_dub_presets.py            | Migrate XOverdub (Overdub/Dub) preset parameters                        | python3 Tools/fix_dub_presets.py [--dry-run]                                                         |
| fix_overworld_presets.py      | fix_overworld_presets.py                                                | python fix_overworld_presets.py                                                                      |
| fix_xobese_xpms.py            | Patches broken keygroup programs to match MPC conventions.              | python3 fix_xobese_xpms.py [--dry-run]                                                               |
| migrate_xocmeta_to_xometa.py  | OddfeliX .xocmeta → XOceanus .xometa Migration Script                   | python3 migrate_xocmeta_to_xometa.py [--dry-run]                                                     |
| xpn_bulk_metadata_editor.py   | XO_OX Designs                                                           | python xpn_bulk_metadata_editor.py                                                                   |
| xpn_entangled_preset_fixer.py | Add second-engine coupling to single-engine Entangled presets           | python xpn_entangled_preset_fixer.py <presets_dir> [--engine FILTER] [--dry-run] [--apply] [--format |
| xpn_pack_version_bumper.py    | XO_OX Designs                                                           | python xpn_pack_version_bumper.py <pack.xpn> --bump patch|minor|major                                |
| xpn_preset_tag_normalizer.py  | Standardize tags across .xometa preset files                            | python3 xpn_preset_tag_normalizer.py <presets_dir> [options]                                         |
| xpn_program_renamer.py        | Batch-rename XPM programs inside a .xpn ZIP archive.                    | # List all program names in a .xpn                                                                   |
| xpn_smart_trim.py             | Automatic sample trimming and loop point detection tool.                | python xpn_smart_trim.py input.wav --output trimmed.wav [--mode one-shot|loop|auto] [--verbose]      |


### Analysis (8 tools)

| Tool                          | Purpose                                                   | CLI Usage                                                                                           |
| ----------------------------- | --------------------------------------------------------- | --------------------------------------------------------------------------------------------------- |
| xpn_kit_theme_analyzer.py     | Sonic Theme Analyzer for drum kit .xpn packs              | python xpn_kit_theme_analyzer.py <pack.xpn> [--format text|json] [--program NAME]                   |
| xpn_optic_fingerprint.py      | XO_OX Designs                                             | # Analyze a directory of WAVs                                                                       |
| xpn_pack_analytics.py         | XO_OX XPN fleet catalog analytics                         | python xpn_pack_analytics.py --packs-dir ./dist [--format text|json|markdown] [--output report.txt] |
| xpn_pack_compare_report.py    | XPN Pack Side-by-Side Comparison Report                   | python xpn_pack_compare_report.py <pack_a.xpn> <pack_b.xpn>                                         |
| xpn_pack_health_dashboard.py  | XO_OX Pack Catalog Health Dashboard                       | python xpn_pack_health_dashboard.py <packs_dir> [--format text|json|markdown] [--output FILE]       |
| xpn_pack_score.py             | XPN Pack Composite Quality Scorer                         | python xpn_pack_score.py <pack.xpn> [--format text|json] [--strict]                                 |
| xpn_sample_category_report.py | XO_OX Designs                                             | python xpn_sample_category_report.py <pack.xpn>                                                     |
| xpn_sample_fingerprinter.py   | Perceptual near-duplicate detector for .xpn sample packs. | python xpn_sample_fingerprinter.py --xpn pack.xpn [--threshold 4] [--format text|json] [--show-all] |


### Search / Browse (5 tools)

| Tool                        | Purpose                                         | CLI Usage                                                                          |
| --------------------------- | ----------------------------------------------- | ---------------------------------------------------------------------------------- |
| find_missing_dna.py         | Find Presets Missing Sonic DNA Blocks           | python3 Tools/find_missing_dna.py                                                  |
| xpn_auto_root_detect.py     | Auto-detect fundamental pitch of a WAV sample.  | python xpn_auto_root_detect.py sample.wav [--verbose]                              |
| xpn_classify_instrument.py  | Rule-based instrument classifier for WAV audio. | python xpn_classify_instrument.py audio.wav                                        |
| xpn_preset_browser.py       | Terminal browser for .xometa preset files.      | python xpn_preset_browser.py --presets-dir ./Presets/XOceanus/ list --engine ONSET |
| xpn_preset_search_engine.py | Full-text search across .xometa preset library  | python xpn_preset_search_engine.py <presets_dir> <query> [options]                 |


### Utility / Misc (45 tools)

| Tool                           | Purpose                                                             | CLI Usage                                                                                            |
| ------------------------------ | ------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| compute_preset_dna.py          | XOceanus Sonic DNA Fingerprint Generator                            | python3 compute_preset_dna.py [--dry-run] [--report]                                                 |
| extract_cpp_presets.py         | Universal C++ → .xometa Preset Extractor                            | python3 extract_cpp_presets.py [--engine xoverdub|xodyssey|xoblongbob|all] [--dry-run]               |
| rename_weak_presets.py         | Fleet-wide preset name elevation for XO_OX-XOceanus                 | python3 rename_weak_presets.py --dry-run    # preview all changes                                    |
| validate_presets.py            | Comprehensive .xometa Quality Assurance                             | python3 validate_presets.py [--fix] [--report] [--strict]                                            |
| xpn_adaptive_velocity.py       | Auto-shapes velocity curves in XPM files based on                   | python xpn_adaptive_velocity.py --xpm input.xpm --stems ./stems/ --output output.xpm                 |
| xpn_attractor_kit.py           | XO_OX Designs                                                       | python xpn_attractor_kit.py --attractor lorenz --preset classic --output ./out/                      |
| xpn_auto_dna.py                | Automatically computes XOceanus 6D Sonic DNA values from WAV audio. | python xpn_auto_dna.py sample.wav                                                                    |
| xpn_braille_rhythm_kit.py      | XO_OX Designs                                                       | python xpn_braille_rhythm_kit.py "hello world" --output ./kits/                                      |
| xpn_ca_presets_kit.py          | XO_OX Designs                                                       | python xpn_ca_presets_kit.py --output ./kits/                                                        |
| xpn_choke_group_assigner.py    | Assign choke/mute groups to drum XPM programs.                      | python xpn_choke_group_assigner.py --xpm drum_kit.xpm --preset onset                                 |
| xpn_climate_kit.py             | XO_OX Designs                                                       | python xpn_climate_kit.py --output ./kits/                                                           |
| xpn_coupling_recipes.py        | XO_OX Designs                                                       | # All coupled presets in a mood folder                                                               |
| xpn_cover_art_batch.py         | XO_OX Designs                                                       | python xpn_cover_art_batch.py --manifest packs.json --output-dir ./covers/ [--size 1400]             |
| xpn_curiosity_engine.py        | XO_OX Designs                                                       | python xpn_curiosity_engine.py --mode contradiction --seed-dna '{"aggression": 0.9}' --pads 16       |
| xpn_dna_interpolator.py        | XO_OX Designs                                                       | python xpn_dna_interpolator.py start.xometa end.xometa \                                             |
| xpn_entropy_kit.py             | XO_OX Designs                                                       | python xpn_entropy_kit.py --input ./samples/ --method max-entropy --count 16 --output ./out/         |
| xpn_gene_kit.py                | XO_OX Designs                                                       | python xpn_gene_kit.py --gene tp53 --output ./out/                                                   |
| xpn_git_kit.py                 | XO_OX Designs                                                       | python xpn_git_kit.py --repo . --output ./out/ --max-commits 200                                     |
| xpn_gravitational_wave_kit.py  | XO_OX Designs                                                       | python xpn_gravitational_wave_kit.py --event GW150914 --phase inspiral --output ./out/               |
| xpn_kit_completeness.py        | Score a drum kit's completeness by analyzing its instrument slots.  | python xpn_kit_completeness.py --xpm kit.xpm --stems ./stems/ [--archetype full|minimal|percussion]  |
| xpn_liner_notes.py             | XO_OX Designs                                                       | # Full collection liner notes (all quads)                                                            |
| xpn_lsystem_kit.py             | XO_OX Designs                                                       | python xpn_lsystem_kit.py --system koch --iterations 3 --output ./kits/                              |
| xpn_monster_rancher.py         | XO_OX Designs                                                       | python xpn_monster_rancher.py source.wav --output ./output/ --mode auto                              |
| xpn_normalize.py               | Sample level normalization for XPN/MPC pack production.             | python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode peak                          |
| xpn_pack_diff.py               | XO_OX Designs                                                       | python xpn_pack_diff.py --old OBESEv1.0.xpn --new OBESEv1.1.xpn                                      |
| xpn_pack_readme_updater.py     | XO_OX Pack README Auto-Section Updater                              | python xpn_pack_readme_updater.py --readme ./README.md [options]                                     |
| xpn_pack_registry.py           | XO_OX XPN Pack Registry Manager                                     | python xpn_pack_registry.py add --pack-name "Iron Machines" --engine ONSET       --mood Foundation - |
| xpn_pack_release_checklist.py  | XO_OX Designs                                                       | python xpn_pack_release_checklist.py <pack.xpn> [--tier signal|form|doctrine]         [--format text |
| xpn_pack_size_optimizer.py     | (no docstring)                                                      | python xpn_pack_size_optimizer.py                                                                    |
| xpn_pad_label_optimizer.py     | XO_OX Designs                                                       | # Preview changes without writing                                                                    |
| xpn_pad_note_mapper.py         | XO_OX Designs                                                       | python xpn_pad_note_mapper.py --note 36                                                              |
| xpn_pendulum_kit.py            | XO_OX Designs                                                       | python xpn_pendulum_kit.py --preset synchronized --output ./out/                                     |
| xpn_poetry_kit.py              | XO_OX Designs                                                       | python xpn_poetry_kit.py --poem shakespeare --output ./out/                                          |
| xpn_preset_dna_recalibrator.py | XPN Preset DNA Recalibrator                                         | python xpn_preset_dna_recalibrator.py <presets_dir> [--engine FILTER]                                |
| xpn_preset_mood_rebalancer.py  | XOceanus Preset Mood Rebalancer                                     | python xpn_preset_mood_rebalancer.py <presets_dir> [--engine FILTER] [--suggest] [--apply] [--format |
| xpn_program_type_classifier.py | XPN program type classifier.                                        | python xpn_program_type_classifier.py <pack.xpn> [--format text|json] [--verbose]                    |
| xpn_sample_rename_batch.py     | XO_OX Designs                                                       | # Dry-run with auto-naming (engine inferred from pack metadata or --engine flag)                     |
| xpn_seismograph_kit.py         | XO_OX Designs                                                       | python xpn_seismograph_kit.py --dataset tohoku --output ./out/                                       |
| xpn_session_handoff.py         | XPN Tool Suite Session Handoff Generator                            | python xpn_session_handoff.py [--output PATH] [--tools-dir DIR] [--specs-dir DIR]                    |
| xpn_to_sfz.py                  | XPN Keygroup → SFZ Converter                                        | python xpn_to_sfz.py --xpn pack.xpn --program "Preset Name" --output ./sfz/                          |
| xpn_transit_kit.py             | XO_OX Designs                                                       | python xpn_transit_kit.py --output ./kits/                                                           |
| xpn_tuning_systems.py          | XO_OX Designs                                                       | python xpn_tuning_systems.py program.xpm --tuning maqam_rast --output ./tuned/                       |
| xpn_turbulence_kit.py          | XO_OX Designs                                                       | python xpn_turbulence_kit.py --scale inertial --output ./out/                                        |
| xpn_velocity_map_visualizer.py | XO_OX Designs                                                       | python xpn_velocity_map_visualizer.py path/to/program.xpm [--no-color] [--strict]                    |
| xpn_xometa_to_pack_bridge.py   | XO_OX Designs                                                       | python xpn_xometa_to_pack_bridge.py \                                                                |


## R&D Spec Inventory (126 files)

All files in `Docs/specs/`. Title extracted from first `#` heading.


| File                                    | Title                                                                |
| --------------------------------------- | -------------------------------------------------------------------- |
| collections_kitchen_engine_concepts.md  | Kitchen Essentials Collection — Engine Concept Specs                 |
| collections_preparation_research.md     | Collections Build Preparation Research                               |
| collections_travel_artwork_concepts.md  | Travel/Water & Artwork/Color Collections — Engine Concept Research   |
| community_marketplace_rnd.md            | Community Marketplace & Creator Economy — R&D                        |
| community_pack_submission_rnd.md        | Community Pack Submission Pipeline — R&D Spec                        |
| coupling_dna_pack_design_rnd.md         | Coupling DNA & XPN Pack Design — R&D                                 |
| engine_sound_design_philosophy_rnd.md   | Engine Sound Design Philosophy — XPN Pack Design Rules               |
| field_guide_posts_pipeline.md           | Field Guide Posts Pipeline — Drafts & Status                         |
| fleet_render_automation_spec.md         | Fleet Render Automation — Technical Specification                    |
| generative_kit_architectures_rnd.md     | Generative Kit Architectures R&D                                     |
| juce_to_mpc_workflow_rnd.md             | JUCE to MPC Workflow: XOceanus to XPN Pack                           |
| kit_curation_innovation_rnd.md          | Kit Curation Innovation — R&D Session                                |
| kit_innovation_master_roadmap.md        | Kit Innovation Master Roadmap                                        |
| live_performance_xpn_rnd.md             | Live Performance XPN — R&D Session                                   |
| mpc35_oxport_integration.md             | MPC 3.5 Features — Oxport Integration Opportunities                  |
| mpc3_advanced_workflow_rnd.md           | MPC OS 3.x Advanced Workflow R&D                                     |
| mpc3_capabilities_research.md           | MPC 3.x Capabilities Research                                        |
| mpc_evolving_capabilities_research.md   | MPC 3.0 / MPCe Evolving Capabilities Research                        |
| mpc_os_35_format_innovations_rnd.md     | MPC OS 3.5 Format Innovations — R&D                                  |
| mpc_vst3_plugin_program_rnd.md          | MPC 3.5 VST3 + Keygroup Synth Engine — R&D Spec                      |
| mpce_native_pack_design.md              | MPCe-Native XPN Pack Design Spec                                     |
| naming_branding_innovation_rnd.md       | Naming & Branding Innovation — R&D Bible                             |
| non_audio_xpn_sources_rnd.md            | Non-Audio Data Sources for XPN Kit Generation — R&D                  |
| overnight_session_2026_03_16.md         | Overnight Session Handoff — 2026-03-16                               |
| overnight_session_2026_03_16_part2.md   | Oxport R&D Session Handoff — March 16, 2026 (Part 2)                 |
| overnight_session_2026_03_16_part3.md   | Oxport R&D Session Handoff — March 16, 2026 (Part 3)                 |
| oxport_adaptive_intelligence_rnd.md     | Oxport Adaptive Intelligence — R&D Spec                              |
| oxport_innovation_round5.md             | Oxport Innovation Round 5 — R&D Spec                                 |
| oxport_v2_architecture_rnd.md           | Oxport v2 Architecture — R&D Document                                |
| oxport_v2_feature_backlog.md            | Oxport V2 Feature Backlog — Prioritized                              |
| pack_cover_art_pipeline_rnd.md          | Pack Cover Art Pipeline — R&D                                        |
| pack_design_onset_drums.md              | ONSET Drum Essentials Pack — Design Spec                             |
| pack_design_xobese_character.md         | XObese Character Pack — Design Spec                                  |
| pack_economics_strategy_rnd.md          | Pack Economics & Release Strategy — R&D                              |
| pack_launch_playbook_rnd.md             | XO_OX XPN Pack Launch Playbook                                       |
| pack_pricing_bundle_strategy_rnd.md     | Pack Pricing, Bundle Design & Revenue Model — R&D                    |
| pack_roadmap_2026_rnd.md                | XO_OX XPN Pack Release Roadmap: April 2026 – March 2027              |
| patreon_content_calendar_rnd.md         | Patreon Content Calendar & Tier Strategy — R&D                       |
| session_handoff_2026-03-16.md           | XPN Tool Suite — Session Handoff                                     |
| site_content_strategy_rnd.md            | XO-OX.org Content Strategy — R&D                                     |
| sound_design_best_practices_xpn.md      | XOceanus — Sound Design Best Practices for XPN/MPC Export            |
| unconventional_kit_ideas_21_40.md       | Unconventional Kit/Keygroup Concepts — Ideas 21–40                   |
| unconventional_xpn_strategies.md        | Unconventional XPN/XPM Output Strategies                             |
| utility_engine_concepts.md              | XOceanus Utility Engine Concepts — R&D Brainstorm                    |
| utility_engine_rapper_bundle.md         | The Rappers — Utility Engine Bundle                                  |
| velocity_science_rnd.md                 | Velocity Science R&D — MPC XPN Pack Design                           |
| world_instrument_dsp_research.md        | World Instrument DSP Research — Collections Preparation              |
| xobserve_competitive_analysis.md        | XObserve — Competitive Analysis and Design Refinement                |
| xobserve_engine_spec.md                 | XObserve Engine — Complete Technical Specification                   |
| xobserve_technical_design.md            | XObserve — Technical Design Document                                 |
| xoptic_oxport_integration_spec.md       | XOptic -> Oxport Integration Spec                                    |
| xoxide_competitive_analysis.md          | XOxide — Competitive Analysis & Design Refinement                    |
| xoxide_engine_spec.md                   | XOxide Engine — Complete Technical Specification                     |
| xoxide_technical_design.md              | XOxide — Technical Design Specification                              |
| xpn_accessibility_rnd.md                | XPN Accessibility R&D                                                |
| xpn_affiliate_partnership_rnd.md        | XPN Affiliate & Partnership R&D                                      |
| xpn_catalog_longevity_rnd.md            | XPN Catalog Longevity — R&D Document                                 |
| xpn_choke_mute_groups_rnd.md            | XPN Choke & Mute Groups R&D                                          |
| xpn_collection_arc_design_rnd.md        | XPN Collection Arc Design — R&D                                      |
| xpn_community_blueprint_rnd.md          | XO_OX Pack Community — Full Launch & Sustainability Blueprint        |
| xpn_community_pack_program_rnd.md       | XPN Community Pack Program — R&D Spec                                |
| xpn_customer_journey_rnd.md             | XPN Pack Customer Journey — R&D Spec                                 |
| xpn_daw_integration_rnd.md              | XPN Pack DAW Integration — R&D Notes                                 |
| xpn_distribution_channels_rnd.md        | XPN Pack Distribution Channels — R&D                                 |
| xpn_distribution_economics_rnd.md       | XPN Distribution Economics — R&D Spec                                |
| xpn_educational_format_rnd.md           | Educational XPN Pack Formats — R&D                                   |
| xpn_educational_pack_rnd.md             | XPN Educational Pack Series — R&D Brief                              |
| xpn_engine_character_briefs_rnd.md      | XPN Engine Character Briefs — Pack Designer Reference                |
| xpn_engine_roadmap_2026_rnd.md          | XPN Engine Roadmap 2026 — R&D Spec                                   |
| xpn_engine_roadmap_rnd.md               | XPN Pack Roadmap — R&D Spec                                          |
| xpn_expansion_tiers_strategy_rnd.md     | XPN Expansion Pack Tier Strategy                                     |
| xpn_field_guide_pack_articles_rnd.md    | XPN Field Guide Articles — R&D Spec                                  |
| xpn_field_guide_series2_articles_rnd.md | Field Guide Series 2: Deep Dives — R&D Spec                          |
| xpn_format_edge_cases_rnd.md            | XPN/XPM Format Edge Cases — R&D Reference                            |
| xpn_format_future_proofing_rnd.md       | XPN/XPM Format Future-Proofing — R&D Spec                            |
| xpn_format_innovations_rnd.md           | XPN Format Innovations — Beyond Standard Pack Design                 |
| xpn_free_pack_strategy_rnd.md           | XPN Free Pack Strategy — R&D Spec                                    |
| xpn_future_engine_pack_concepts_rnd.md  | XPN Pack Concepts — Concept Engines + Utility Engine Selection       |
| xpn_hardware_compatibility_rnd.md       | XPN Hardware Compatibility Matrix — R&D Spec                         |
| xpn_hardware_testing_protocol_rnd.md    | XPN Hardware Testing Protocol — R&D Spec                             |
| xpn_kit_builder_guide_rnd.md            | XPN Kit Builder Guide — R&D Reference                                |
| xpn_label_design_rnd.md                 | XPN Pack Label & Visual Identity System — R&D Spec                   |
| xpn_launch_checklist_v1_rnd.md          | XO_OX Expansion Pack Launch Checklist                                |
| xpn_live_performance_rnd.md             | XPN Live Performance R&D                                             |
| xpn_marketplace_playbook_rnd.md         | XO_OX Expansion Pack Marketplace Playbook                            |
| xpn_metadata_standards_rnd.md           | XPN Metadata Standards — R&D                                         |
| xpn_mpc_os_integration_rnd.md           | XPN / MPC OS Deep Integration R&D                                    |
| xpn_mpce_native_design_guide_rnd.md     | XPN MPCe-Native Expansion Pack Design Guide                          |
| xpn_multipack_bundle_rnd.md             | XPN Multi-Pack Bundle Strategy — R&D                                 |
| xpn_multipack_bundle_strategy_rnd.md    | XPN Multi-Pack Bundle Strategy — R&D Spec                            |
| xpn_narrative_format_rnd.md             | XPN as Narrative Format — R&D Session                                |
| xpn_oxport_v2_roadmap_rnd.md            | Oxport v2 — Unified Session Pipeline: R&D Spec                       |
| xpn_pack_design_templates_rnd.md        | XPN Pack Design Templates — R&D Spec                                 |
| xpn_pack_feedback_loops_rnd.md          | XPN Pack Feedback Loops — R&D Spec                                   |
| xpn_pack_launch_checklist_rnd.md        | XPN Pack Pre-Launch Checklist                                        |
| xpn_pack_naming_brand_rnd.md            | XPN Pack Naming & Brand Guidelines — R&D                             |
| xpn_pack_story_design_rnd.md            | Story-First Pack Design — R&D                                        |
| xpn_pack_testing_protocol_rnd.md        | XPN Pack Hardware Testing Protocol                                   |
| xpn_pack_tier_system_rnd.md             | XPN Pack Tier System — R&D Spec                                      |
| xpn_pack_versioning_rnd.md              | XPN Pack Version Management — R&D Spec                               |
| xpn_patreon_content_calendar_rnd.md     | XPN Patreon Content Strategy & Calendar — R&D Spec                   |
| xpn_patreon_q2_2026_calendar_rnd.md     | XO_OX Patreon Q2 2026 Content Calendar — R&D Spec                    |
| xpn_preset_expansion_research_rnd.md    | XPN Preset Expansion R&D Spec                                        |
| xpn_preset_naming_conventions_rnd.md    | XPN Preset & Program Naming Conventions — R&D Spec                   |
| xpn_preset_variation_rnd.md             | XPN Preset Variation R&D                                             |
| xpn_production_tools_comparison_rnd.md  | Oxport Tool Suite — Competitive Analysis & R&D Spec                  |
| xpn_qa_pipeline_design_rnd.md           | XPN QA Pipeline Design — R&D Spec                                    |
| xpn_qlink_defaults_rnd.md               | XPN Q-Link Defaults R&D                                              |
| xpn_quality_scoring_rnd.md              | XPN Pack Quality Scoring Rubric — R&D Spec                           |
| xpn_render_automation_spec.md           | XPN Render Automation — `renderNoteToWav()` Implementation Spec      |
| xpn_render_quality_settings_rnd.md      | XPN Render Quality Settings — R&D                                    |
| xpn_round_robin_strategies_rnd.md       | XPN Round-Robin Strategies — R&D                                     |
| xpn_sample_curation_philosophy_rnd.md   | XPN Sample Curation Philosophy — R&D Spec                            |
| xpn_sample_library_vs_keygroup_rnd.md   | XPN Pack Format Decision: Sample Library vs Keygroup vs Drum Program |
| xpn_sample_normalization_rnd.md         | XPN Sample Normalization — R&D Spec                                  |
| xpn_sample_pack_curation_rnd.md         | XPN Sample Pack Curation — R&D Spec                                  |
| xpn_seasonal_cultural_themes_rnd.md     | XPN Seasonal & Cultural Themes — R&D                                 |
| xpn_seo_discoverability_rnd.md          | XPN SEO & Discoverability R&D                                        |
| xpn_smart_trim_rnd.md                   | XPN Smart Trim & Loop Point Detection — R&D Spec                     |
| xpn_social_media_strategy_rnd.md        | XO_OX Expansion Pack — Social Media Strategy R&D Spec                |
| xpn_social_proof_community_rnd.md       | XPN Social Proof & Community R&D                                     |
| xpn_sonic_dna_automation_rnd.md         | XPN Sonic DNA Automation — R&D Spec                                  |
| xpn_sound_design_workflow_rnd.md        | XPN Sound Design Workflow — R&D Spec                                 |
| xpn_tutorial_content_rnd.md             | XO_OX Tutorial & Educational Content Strategy — R&D Spec             |
| xpn_tutorial_video_content_rnd.md       | XPN Tutorial Video Content — R&D Spec                                |
| xpn_velocity_map_visualizer_rnd.md      | XPN Velocity Map Visualizer — R&D Spec                               |


## How to Use This Document

- **New to the toolkit?** Start with Core Pipeline — `oxport.py` is the main entry point.

- **Need to validate output?** Check QA / Validation tools.

- **Building a kit?** Start with Build / Export then run a QA pass.

- **R&D specs**: each `*_rnd.md` file is a focused research document on one topic.

- **Re-generate this file**: `python Tools/xpn_session_summary_generator.py`

