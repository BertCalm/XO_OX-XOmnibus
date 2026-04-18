# XPN Tool Suite — Session Handoff

**Date**: 2026-03-16

## Summary Stats

| Metric | Value |
| --- | --- |
| Total xpn_*.py tools | 72 |
| Total lines of tool code | 48,736 |
| Total R&D specs (*_rnd.md) | 72 |
| Total spec lines | 20,121 |

## Tool Catalog

| Tool                              | Lines | Synopsis                                                                         | Last Modified |
| --------------------------------- | ----- | -------------------------------------------------------------------------------- | ------------- |
| xpn_adaptive_velocity.py          | 352   | python xpn_adaptive_velocity.py --xpm input.xpm --stems ./stems/ --output out... | 2026-03-16    |
| xpn_articulation_builder.py       | 971   | # Key-switch mode (low-octave notes C0+ select articulation)                     | 2026-03-16    |
| xpn_attractor_kit.py              | 678   | python xpn_attractor_kit.py --attractor lorenz --preset classic --output ./out/  | 2026-03-16    |
| xpn_auto_dna.py                   | 323   | python xpn_auto_dna.py sample.wav                                                | 2026-03-16    |
| xpn_auto_root_detect.py           | 315   | python xpn_auto_root_detect.py sample.wav [--verbose]                            | 2026-03-16    |
| xpn_batch_export.py               | 371   | python xpn_batch_export.py --config batch.json                                   | 2026-03-16    |
| xpn_braille_rhythm_kit.py         | 862   | python xpn_braille_rhythm_kit.py "hello world" --output ./kits/                  | 2026-03-16    |
| xpn_bundle_builder.py             | 1383  | # Build from a bundle profile JSON                                               | 2026-03-16    |
| xpn_ca_presets_kit.py             | 641   | python xpn_ca_presets_kit.py --output ./kits/                                    | 2026-03-16    |
| xpn_changelog_generator.py        | 349   | python xpn_changelog_generator.py --diff-mode --old v1.0.xpn --new v1.1.xpn      | 2026-03-16    |
| xpn_choke_group_assigner.py       | 404   | python xpn_choke_group_assigner.py --xpm drum_kit.xpm --preset onset             | 2026-03-16    |
| xpn_classify_instrument.py        | 547   | python xpn_classify_instrument.py audio.wav                                      | 2026-03-16    |
| xpn_climate_kit.py                | 1238  | python xpn_climate_kit.py --output ./kits/                                       | 2026-03-16    |
| xpn_community_qa.py               | 708   | python xpn_community_qa.py --submission pack.submission.zip [--discord-output... | 2026-03-16    |
| xpn_complement_renderer.py        | 1163  | # Single engine, discover presets in a directory                                 | 2026-03-16    |
| xpn_coupling_docs_generator.py    | 223   | python xpn_coupling_docs_generator.py --xometa preset.xometa                     | 2026-03-16    |
| xpn_coupling_recipes.py           | 746   | # All coupled presets in a mood folder                                           | 2026-03-16    |
| xpn_cover_art.py                  | 1045  | from xpn_cover_art import generate_cover                                         | 2026-03-16    |
| xpn_cover_art_audit.py            | 313   | python xpn_cover_art_audit.py --xpn pack.xpn [--strict] [--format text|json]     | 2026-03-16    |
| xpn_cover_art_batch.py            | 631   | python xpn_cover_art_batch.py --manifest packs.json --output-dir ./covers/ [-... | 2026-03-16    |
| xpn_curiosity_engine.py           | 1121  | python xpn_curiosity_engine.py --mode contradiction --seed-dna '{"aggression"... | 2026-03-16    |
| xpn_deconstruction_builder.py     | 1002  | -                                                                                | 2026-03-16    |
| xpn_dedup_checker.py              | 321   | python xpn_dedup_checker.py --index known_hashes.json                            | 2026-03-16    |
| xpn_drum_export.py                | 1067  | # Export with velocity layers (default)                                          | 2026-03-16    |
| xpn_entropy_kit.py                | 840   | python xpn_entropy_kit.py --input ./samples/ --method max-entropy --count 16 ... | 2026-03-16    |
| xpn_evolution_builder.py          | 926   | python xpn_evolution_builder.py \                                                | 2026-03-16    |
| xpn_full_qa_runner.py             | 400   | python xpn_full_qa_runner.py --xpn pack.xpn [--tools-dir ./Tools]         [--... | 2026-03-16    |
| xpn_gene_kit.py                   | 735   | python xpn_gene_kit.py --gene tp53 --output ./out/                               | 2026-03-16    |
| xpn_git_kit.py                    | 857   | python xpn_git_kit.py --repo . --output ./out/ --max-commits 200                 | 2026-03-16    |
| xpn_gravitational_wave_kit.py     | 637   | python xpn_gravitational_wave_kit.py --event GW150914 --phase inspiral --outp... | 2026-03-16    |
| xpn_keygroup_export.py            | 1051  | -                                                                                | 2026-03-16    |
| xpn_kit_completeness.py           | 525   | python xpn_kit_completeness.py --xpm kit.xpm --stems ./stems/ [--archetype fu... | 2026-03-16    |
| xpn_kit_expander.py               | 579   | # Expand single WAV into velocity layers                                         | 2026-03-15    |
| xpn_kit_validator.py              | 509   | # Validate a single .xpm file                                                    | 2026-03-16    |
| xpn_liner_notes.py                | 2034  | # Full collection liner notes (all quads)                                        | 2026-03-16    |
| xpn_lsystem_kit.py                | 914   | python xpn_lsystem_kit.py --system koch --iterations 3 --output ./kits/          | 2026-03-16    |
| xpn_manifest_generator.py         | 283   | python xpn_manifest_generator.py --pack-name "OBESE Rising" --engine OBESE   ... | 2026-03-16    |
| xpn_manifest_validator.py         | 307   | python xpn_manifest_validator.py --xpn pack.xpn [--strict] [--format text|json]  | 2026-03-16    |
| xpn_monster_rancher.py            | 1662  | python xpn_monster_rancher.py source.wav --output ./output/ --mode auto          | 2026-03-16    |
| xpn_mpce_quad_builder.py          | 1160  | python xpn_mpce_quad_builder.py \                                                | 2026-03-16    |
| xpn_normalize.py                  | 370   | python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode peak      | 2026-03-16    |
| xpn_optic_fingerprint.py          | 648   | # Analyze a directory of WAVs                                                    | 2026-03-16    |
| xpn_pack_analytics.py             | 452   | python xpn_pack_analytics.py --packs-dir ./dist [--format text|json|markdown]... | 2026-03-16    |
| xpn_pack_diff.py                  | 423   | python xpn_pack_diff.py --old OBESEv1.0.xpn --new OBESEv1.1.xpn                  | 2026-03-16    |
| xpn_pack_readme_generator.py      | 350   | python xpn_pack_readme_generator.py         --pack-name "ONSET Ritual Drums" ... | 2026-03-16    |
| xpn_pack_readme_updater.py        | 226   | python xpn_pack_readme_updater.py --readme ./README.md [options]                 | 2026-03-16    |
| xpn_pack_registry.py              | 363   | python xpn_pack_registry.py add --pack-name "Iron Machines" --engine ONSET   ... | 2026-03-16    |
| xpn_pack_score.py                 | 861   | python xpn_pack_score.py --xpn pack.xpn [--stems ./stems/] [--json]              | 2026-03-16    |
| xpn_packager.py                   | 373   | from xpn_packager import package_xpn, XPNMetadata                                | 2026-03-16    |
| xpn_pendulum_kit.py               | 595   | python xpn_pendulum_kit.py --preset synchronized --output ./out/                 | 2026-03-16    |
| xpn_poetry_kit.py                 | 946   | python xpn_poetry_kit.py --poem shakespeare --output ./out/                      | 2026-03-16    |
| xpn_preset_variation_generator.py | 564   | python xpn_preset_variation_generator.py --xpm base.xpm \                        | 2026-03-16    |
| xpn_preview_generator.py          | 723   | # Generate preview for a drum pack                                               | 2026-03-16    |
| xpn_program_renamer.py            | 290   | # List all program names in a .xpn                                               | 2026-03-16    |
| xpn_qa_checker.py                 | 353   | python xpn_qa_checker.py samples/kick_vel_01.wav                                 | 2026-03-16    |
| xpn_render_spec.py                | 611   | # Generate render spec for one preset                                            | 2026-03-15    |
| xpn_sample_audit.py               | 462   | python xpn_sample_audit.py --xpn pack.xpn [--strict] [--format text|json]        | 2026-03-16    |
| xpn_sample_categorizer.py         | 365   | # Classify and print results                                                     | 2026-03-15    |
| xpn_seismograph_kit.py            | 812   | python xpn_seismograph_kit.py --dataset tohoku --output ./out/                   | 2026-03-16    |
| xpn_session_handoff.py            | 237   | python xpn_session_handoff.py [--output PATH] [--tools-dir DIR] [--specs-dir ... | 2026-03-16    |
| xpn_setlist_builder.py            | 508   | python xpn_setlist_builder.py --packs-dir ./dist --set-length 8 \                | 2026-03-16    |
| xpn_smart_trim.py                 | 555   | python xpn_smart_trim.py input.wav --output trimmed.wav [--mode one-shot|loop... | 2026-03-16    |
| xpn_stems_checker.py              | 831   | python xpn_stems_checker.py --stems ./stems/ --output report.json                | 2026-03-16    |
| xpn_submission_packager.py        | 672   | python xpn_submission_packager.py \                                              | 2026-03-16    |
| xpn_to_sfz.py                     | 693   | python xpn_to_sfz.py --xpn pack.xpn --program "Preset Name" --output ./sfz/      | 2026-03-16    |
| xpn_transit_kit.py                | 741   | python xpn_transit_kit.py --output ./kits/                                       | 2026-03-16    |
| xpn_tuning_systems.py             | 767   | python xpn_tuning_systems.py program.xpm --tuning maqam_rast --output ./tuned/   | 2026-03-16    |
| xpn_turbulence_kit.py             | 569   | python xpn_turbulence_kit.py --scale inertial --output ./out/                    | 2026-03-16    |
| xpn_tutorial_builder.py           | 878   | python xpn_tutorial_builder.py \                                                 | 2026-03-16    |
| xpn_validator.py                  | 991   | # Validate a single .xpn file                                                    | 2026-03-15    |
| xpn_variation_generator.py        | 881   | python xpn_variation_generator.py my_pack.xpn --type tone --direction felix -... | 2026-03-16    |
| xpn_velocity_map_visualizer.py    | 433   | python xpn_velocity_map_visualizer.py path/to/program.xpm [--no-color] [--str... | 2026-03-16    |

## R&D Spec Catalog

| Spec File                              | Title                                                                | Lines |
| -------------------------------------- | -------------------------------------------------------------------- | ----- |
| community_marketplace_rnd.md           | Community Marketplace & Creator Economy — R&D                        | 357   |
| community_pack_submission_rnd.md       | Community Pack Submission Pipeline — R&D Spec                        | 131   |
| coupling_dna_pack_design_rnd.md        | Coupling DNA & XPN Pack Design — R&D                                 | 339   |
| engine_sound_design_philosophy_rnd.md  | Engine Sound Design Philosophy — XPN Pack Design Rules               | 193   |
| generative_kit_architectures_rnd.md    | Generative Kit Architectures R&D                                     | 713   |
| juce_to_mpc_workflow_rnd.md            | JUCE to MPC Workflow: XOceanus to XPN Pack                           | 205   |
| kit_curation_innovation_rnd.md         | Kit Curation Innovation — R&D Session                                | 1005  |
| live_performance_xpn_rnd.md            | Live Performance XPN — R&D Session                                   | 881   |
| mpc3_advanced_workflow_rnd.md          | MPC OS 3.x Advanced Workflow R&D                                     | 103   |
| mpc_os_35_format_innovations_rnd.md    | MPC OS 3.5 Format Innovations — R&D                                  | 138   |
| mpc_vst3_plugin_program_rnd.md         | MPC 3.5 VST3 + Keygroup Synth Engine — R&D Spec                      | 577   |
| naming_branding_innovation_rnd.md      | Naming & Branding Innovation — R&D Bible                             | 644   |
| non_audio_xpn_sources_rnd.md           | Non-Audio Data Sources for XPN Kit Generation — R&D                  | 571   |
| oxport_adaptive_intelligence_rnd.md    | Oxport Adaptive Intelligence — R&D Spec                              | 237   |
| oxport_v2_architecture_rnd.md          | Oxport v2 Architecture — R&D Document                                | 181   |
| pack_cover_art_pipeline_rnd.md         | Pack Cover Art Pipeline — R&D                                        | 163   |
| pack_economics_strategy_rnd.md         | Pack Economics & Release Strategy — R&D                              | 306   |
| pack_launch_playbook_rnd.md            | XO_OX XPN Pack Launch Playbook                                       | 131   |
| pack_pricing_bundle_strategy_rnd.md    | Pack Pricing, Bundle Design & Revenue Model — R&D                    | 275   |
| pack_roadmap_2026_rnd.md               | XO_OX XPN Pack Release Roadmap: April 2026 – March 2027              | 104   |
| patreon_content_calendar_rnd.md        | Patreon Content Calendar & Tier Strategy — R&D                       | 298   |
| site_content_strategy_rnd.md           | XO-OX.org Content Strategy — R&D                                     | 224   |
| velocity_science_rnd.md                | Velocity Science R&D — MPC XPN Pack Design                           | 132   |
| xpn_accessibility_rnd.md               | XPN Accessibility R&D                                                | 94    |
| xpn_affiliate_partnership_rnd.md       | XPN Affiliate & Partnership R&D                                      | 74    |
| xpn_catalog_longevity_rnd.md           | XPN Catalog Longevity — R&D Document                                 | 68    |
| xpn_choke_mute_groups_rnd.md           | XPN Choke & Mute Groups R&D                                          | 164   |
| xpn_collection_arc_design_rnd.md       | XPN Collection Arc Design — R&D                                      | 265   |
| xpn_community_blueprint_rnd.md         | XO_OX Pack Community — Full Launch & Sustainability Blueprint        | 481   |
| xpn_customer_journey_rnd.md            | XPN Pack Customer Journey — R&D Spec                                 | 214   |
| xpn_daw_integration_rnd.md             | XPN Pack DAW Integration — R&D Notes                                 | 96    |
| xpn_distribution_channels_rnd.md       | XPN Pack Distribution Channels — R&D                                 | 146   |
| xpn_distribution_economics_rnd.md      | XPN Distribution Economics — R&D Spec                                | 340   |
| xpn_educational_format_rnd.md          | Educational XPN Pack Formats — R&D                                   | 643   |
| xpn_educational_pack_rnd.md            | XPN Educational Pack Series — R&D Brief                              | 91    |
| xpn_engine_roadmap_rnd.md              | XPN Pack Roadmap — R&D Spec                                          | 405   |
| xpn_format_edge_cases_rnd.md           | XPN/XPM Format Edge Cases — R&D Reference                            | 127   |
| xpn_format_future_proofing_rnd.md      | XPN/XPM Format Future-Proofing — R&D Spec                            | 381   |
| xpn_format_innovations_rnd.md          | XPN Format Innovations — Beyond Standard Pack Design                 | 467   |
| xpn_free_pack_strategy_rnd.md          | XPN Free Pack Strategy — R&D                                         | 354   |
| xpn_future_engine_pack_concepts_rnd.md | XPN Pack Concepts — Concept Engines + Utility Engine Selection       | 289   |
| xpn_hardware_compatibility_rnd.md      | XPN Hardware Compatibility Matrix — R&D Spec                         | 477   |
| xpn_label_design_rnd.md                | XPN Pack Label & Visual Identity System — R&D Spec                   | 281   |
| xpn_live_performance_rnd.md            | XPN Live Performance R&D                                             | 137   |
| xpn_metadata_standards_rnd.md          | XPN Metadata Standards — R&D                                         | 227   |
| xpn_mpc_os_integration_rnd.md          | XPN / MPC OS Deep Integration R&D                                    | 350   |
| xpn_multipack_bundle_rnd.md            | XPN Multi-Pack Bundle Strategy — R&D                                 | 282   |
| xpn_narrative_format_rnd.md            | XPN as Narrative Format — R&D Session                                | 414   |
| xpn_oxport_v2_roadmap_rnd.md           | Oxport v2 — Unified Session Pipeline: R&D Spec                       | 392   |
| xpn_pack_feedback_loops_rnd.md         | XPN Pack Feedback Loops — R&D Spec                                   | 79    |
| xpn_pack_launch_checklist_rnd.md       | XPN Pack Pre-Launch Checklist                                        | 168   |
| xpn_pack_naming_brand_rnd.md           | XPN Pack Naming & Brand Guidelines — R&D                             | 131   |
| xpn_pack_tier_system_rnd.md            | XPN Pack Tier System — R&D Spec                                      | 122   |
| xpn_pack_versioning_rnd.md             | XPN Pack Version Management — R&D Spec                               | 306   |
| xpn_patreon_content_calendar_rnd.md    | XPN Patreon Content Strategy & Calendar — R&D Spec                   | 252   |
| xpn_preset_naming_conventions_rnd.md   | XPN Preset & Program Naming Conventions — R&D Spec                   | 451   |
| xpn_preset_variation_rnd.md            | XPN Preset Variation R&D                                             | 113   |
| xpn_production_tools_comparison_rnd.md | Oxport Tool Suite — Competitive Analysis & R&D Spec                  | 376   |
| xpn_qlink_defaults_rnd.md              | XPN Q-Link Defaults R&D                                              | 194   |
| xpn_quality_scoring_rnd.md             | XPN Pack Quality Scoring Rubric — R&D Spec                           | 146   |
| xpn_render_quality_settings_rnd.md     | XPN Render Quality Settings — R&D                                    | 124   |
| xpn_round_robin_strategies_rnd.md      | XPN Round-Robin Strategies — R&D                                     | 124   |
| xpn_sample_library_vs_keygroup_rnd.md  | XPN Pack Format Decision: Sample Library vs Keygroup vs Drum Program | 123   |
| xpn_sample_normalization_rnd.md        | XPN Sample Normalization — R&D Spec                                  | 297   |
| xpn_seasonal_cultural_themes_rnd.md    | XPN Seasonal & Cultural Themes — R&D                                 | 74    |
| xpn_seo_discoverability_rnd.md         | XPN SEO & Discoverability R&D                                        | 123   |
| xpn_smart_trim_rnd.md                  | XPN Smart Trim & Loop Point Detection — R&D Spec                     | 203   |
| xpn_social_proof_community_rnd.md      | XPN Social Proof & Community R&D                                     | 91    |
| xpn_sonic_dna_automation_rnd.md        | XPN Sonic DNA Automation — R&D Spec                                  | 244   |
| xpn_sound_design_workflow_rnd.md       | XPN Sound Design Workflow — End-to-End R&D Spec                      | 436   |
| xpn_tutorial_content_rnd.md            | XO_OX Tutorial & Educational Content Strategy — R&D Spec             | 398   |
| xpn_velocity_map_visualizer_rnd.md     | XPN Velocity Map Visualizer — R&D Spec                               | 379   |

## Next Steps

_Fill in before handing off to next session._

- [ ] 
