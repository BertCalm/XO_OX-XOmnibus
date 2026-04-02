# Archived Tools — Move Log

This file documents tools moved to _archive/ from Tools/ root during the Phase D dead-code prune (2026-03-29).

## Criteria

A file was moved to _archive/ if ALL of the following are true:
1. Not imported (directly or lazily) by `oxport.py` or any of its pipeline dependencies
2. Does NOT have `if __name__ == "__main__"` (i.e., not a standalone user-runnable CLI tool)

Files with `__main__` are kept in Tools/ even if not used by the pipeline — the user may invoke them directly.

---

## xoutshine.py (archived 2026-03-29)
Standalone Python duplicate of the C++ XOutshine DSP engine. Superseded by:
- Desktop: Originate UI wizard (`Source/Export/XOriginate.h` + `Source/UI/Outshine/`)
- CLI: Oxport pipeline (`Tools/oxport.py`)

See `Docs/export-architecture.md` for the full Export Pyramid architecture.

---

## Files Moved (Phase E, 2026-03-31 — Issue #94)

14 one-shot numbered-fix and migration scripts. All have no `__main__`, are not imported by any other tool, and represent completed single-issue patches.

| File | Reason |
|------|--------|
| `fix_261_quarantine_reason.py` | Closes #261 — adds default quarantine_reason to quarantined presets. One-time fix, already applied. |
| `fix_264_standardize_folder_structure.py` | Closes #264 — standardizes preset folder structure. One-time fix, already applied. |
| `fix_425_447_macro_format.py` | Closes #425, #447 — fixes deprecated macro format and missing macroLabels. One-time fix. |
| `fix_428_phantom_engine_declarations.py` | Closes #428 — removes phantom engine declarations. One-time fix. |
| `fix_446_macro_prefix_migration.py` | Closes #446 — restores macro parameter values stripped by fix_preset_macros.py. One-time recovery. |
| `fix_448_same_mood_dups.py` | Closes #448 — fixes 8 same-mood duplicate preset names. One-time fix. |
| `fix_drift_adsr_units.py` | One-time ADSR unit normalization for Drift engine. Superseded by preset schema validation. |
| `fix_duplicate_names.py` | One-time duplicate name sweep. Superseded by `xpn_preset_duplicate_detector.py`. |
| `fix_f4_audit.py` | F4 sound designer audit fixes — one-time rename and tier assignment pass. |
| `fix_preset_jargon.py` | One-time preset name jargon cleanup. Superseded by `xpn_preset_name_generator.py`. |
| `fix_preset_macros.py` | One-time macro dict migration (v1 → v2 format). Superseded by `validate_presets.py`. |
| `fix_preset_schema.py` | One-time schema migration runner. Superseded by `xoceanus_preset_migration.py` (now also archived). |
| `migrate_onset_macro_labels.py` | One-time ONSET macro label migration. Engine-specific, already applied. |
| `xoceanus_preset_migration.py` | Two-pass coupling/schema migration (bare-list fix, key renaming, couplingIntensity normalization). Applied 2026-03-22. Superseded by `validate_presets.py` + `xpn_xometa_schema_version_migrator.py`. |

---

## Files Moved (Phase D, 2026-03-29)

| File | Reason |
|------|--------|
| `generate_broth_presets.py` | No `__main__`, not imported by pipeline. One-time Broth quad preset generator (Kitchen Collection, 2026-03-22). |
| `xpn_per_mood_gap_report.py` | No `__main__`, not imported by pipeline. One-time DNA gap reporting helper; superseded by `xpn_dna_gap_finder.py`. |

---

## Files Kept in Tools/ (Active Pipeline Modules)

These are imported by `oxport.py` (directly or lazily) — do not archive:

- `xpn_render_spec.py` — Stage 1: render spec generation
- `xpn_sample_categorizer.py` — Stage 2: WAV voice classification
- `xpn_kit_expander.py` — Stage 3: drum kit velocity/cycle expansion
- `xpn_qa_checker.py` — Stage QA: perceptual WAV quality checks
- `xpn_smart_trim.py` — Stage smart_trim: silence tail removal
- `xpn_drum_export.py` — Stage export (drum): XPM generation
- `xpn_keygroup_export.py` — Stage export (keygroup): XPM generation
- `xpn_adaptive_velocity.py` — DNA velocity sculpting (Legend Feature #1)
- `xpn_classify_instrument.py` — Instrument classifier for DNA velocity
- `xpn_choke_group_assigner.py` — Optional choke group assignment post-export
- `xpn_cover_art.py` — Stage cover_art: procedural cover art (Pillow-based)
- `xpn_cover_art_generator_v2.py` — Stage art (build command): v2 cover art generator
- `xpn_manifest_generator.py` — expansion.json + bundle_manifest.json generation
- `xpn_packager.py` — Stage package: .xpn ZIP assembly
- `xpn_auto_dna.py` — Per-sample 6D Sonic DNA computation
- `xpn_tuning_systems.py` — Microtonal tuning system library
- `xpn_loudness_ledger.py` — Cross-pack loudness ledger (Legend Feature #6)
- `xpn_batch_export.py` — Batch pipeline runner (batch command)
- `xpn_mpce_quad_builder.py` — MPCe quad-corner XPM builder (assemble stage)
- `xpn_profile_preset_selector.py` — DNA-diversity preset selection (select stage)
- `xpn_profile_validator.py` — Profile phenotype validation (validate stage)
- `xpn_intent_generator.py` — xpn_intent.json sidecar generation
- `xpn_validator.py` — .xpn archive structural validation
- `xpn_qa_decision_log.py` — QA accept/reject log (used by profile_preset_selector)
- `validate_presets.py` — 10-point .xometa preset validation (validate command)
- `oxport_render.py` — Real-time MIDI+audio capture via BlackHole (render stage)

## Standalone CLI Tools (kept in Tools/ — have __main__)

166+ tools with `if __name__ == "__main__"` remain in Tools/ root. These are directly user-runnable even though the pipeline does not import them. Examples:
- `add_missing_dna.py`, `audit_sonic_dna.py`, `breed_presets.py` — preset DNA tools
- `xpn_preset_browser.py`, `xpn_fleet_health_dashboard.py` — fleet analysis
- `xpn_coupling_recipes.py`, `xpn_coupling_pair_recommender.py` — coupling utilities
- Engine-specific preset generators: `xpn_obrix_preset_gen.py`, `xpn_ostinato_preset_gen.py`, etc.

See Tools/README.md for the active pipeline overview.
