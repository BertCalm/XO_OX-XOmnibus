# Archived Tools — Move Log

This file documents tools moved to _archive/ from Tools/ root during the Phase D dead-code prune (2026-03-29).

## Criteria

A file was moved to _archive/ if ALL of the following are true:
1. Not imported (directly or lazily) by `oxport.py` or any of its pipeline dependencies
2. Does NOT have `if __name__ == "__main__"` (i.e., not a standalone user-runnable CLI tool)

Files with `__main__` are kept in Tools/ even if not used by the pipeline — the user may invoke them directly.

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
