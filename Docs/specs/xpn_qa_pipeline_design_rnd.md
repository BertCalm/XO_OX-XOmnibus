# XPN QA Pipeline Design — R&D Spec
**Date**: 2026-03-16
**Status**: Draft

---

## 1. Pipeline Overview

The XPN QA pipeline is a 5-stage funnel that every expansion pack passes through before release. Each stage builds on the last — a pack that fails Stage 1 does not proceed to Stage 2.

**Stage 1: Structure Check**
Confirms the pack is well-formed before any deeper analysis. `manifest_validator` verifies all required fields (name, version, engine, preset count, DNA tags), checks that every referenced sample file exists, and flags schema violations as ERRORs vs. warnings. `cover_art_audit` confirms image dimensions, format (PNG/JPEG), and that the artwork file is present and not a placeholder.

**Stage 2: Audio Quality**
Inspects the actual sample content. `sample_audit` scans for clipping, silence padding beyond threshold, encoding anomalies, and mismatched sample rates. `tuning_coverage_checker` verifies that chromatic or key-zone coverage has no gaps that would produce out-of-tune playback on the MPC. `layer_balance_checker` confirms that velocity layers have consistent RMS levels and that transitions between layers won't produce obvious volume jumps during performance.

**Stage 3: Design Quality**
Evaluates the musical and structural decisions. `kit_validator` confirms kit mode assignments (velocity/cycle/random/smart) are intentional and well-formed. `pad_label_optimizer` flags pads with generic names ("Sample 1") and suggests label patterns consistent with the pack's DNA. `velocity_map_visualizer` renders a human-readable summary of all velocity zones so a reviewer can spot asymmetries without loading the MPC.

**Stage 4: Pack Identity**
Assesses whether the pack reads as a coherent artifact. `pack_score` aggregates Stage 1–3 findings into a single numeric score (0–100). `quick_preview_generator` renders a short audio preview chain across representative presets, giving a listener-level gut check before the ear test. If the pack belongs to a collection, `collection_arc_validator` confirms that its DNA tags, mood assignments, and naming conventions are consistent with neighboring packs in the set.

**Stage 5: Release Prep**
Finalizes the artifact. `changelog_generator` produces a structured diff between the current version and the prior release manifest. `export_report_generator` writes the full QA summary to a timestamped file. `bulk_metadata_editor` applies the version bump and any last-minute metadata corrections across all presets in a single pass.

---

## 2. Tool Chaining

`full_qa_runner.py` in `Tools/` orchestrates Stages 1–3 in sequence, halting on ERROR-class findings. It accepts a path to a pack directory and writes a JSON results file.

A complete pipeline run looks like this:

```
python Tools/full_qa_runner.py packs/OPAL/ --out results/opal_qa.json
python Tools/pack_score.py results/opal_qa.json
python Tools/quick_preview_generator.py packs/OPAL/ --out previews/opal_preview.wav
python Tools/collection_arc_validator.py packs/OPAL/ --collection collections/constellation.json
python Tools/changelog_generator.py packs/OPAL/ --prev releases/opal_v1.0/
python Tools/export_report_generator.py results/opal_qa.json --out reports/opal_release_report.txt
python Tools/bulk_metadata_editor.py packs/OPAL/ --version 1.1.0
```

Stages 4–5 tools are run manually after passing the Stage 1–3 gate, because they require human sign-off between steps.

---

## 3. Pass/Fail Criteria per Tier

**SIGNAL tier** (functional, shippable for early access):
- `full_qa_runner` score ≥ 70
- Zero manifest ERRORs (warnings acceptable)
- Cover art present (dimensions unchecked)

**FORM tier** (polished, suitable for general release):
- Score ≥ 80
- Cover art ≥ 400px on shortest side
- All sample ERRORs resolved (clipping, silence anomalies, missing files)
- No pad labels flagged as generic

**DOCTRINE tier** (canonical, collection-ready):
- Score ≥ 90
- Velocity layer consistency PASS from `layer_balance_checker`
- Tuning anomalies CLEAR from `tuning_coverage_checker`
- Collection arc validation PASS (if part of a collection)
- Preview audio reviewed and approved

---

## 4. CI Integration

The pipeline can run automatically in GitHub Actions on any push to a `packs/` branch. A matrix strategy iterates over all `.xpn` files changed in the PR. Each job runs `full_qa_runner.py` and `pack_score.py`, then uses the GitHub script action to post the score table as a PR comment. Failures block merge.

The workflow trigger is `push` to branches matching `packs/**` and `pull_request` targeting `main`. The matrix is generated dynamically by listing changed files in the PR. Score results are uploaded as workflow artifacts and summarized inline in the PR check status.

This gives every pack submission a consistent, auditable paper trail — the score comment becomes part of the PR history.

---

## 5. Manual Review Checkpoints

Automated tools catch structure and measurable signal. They cannot catch:

- **Does it sound good?** Load the pack on a physical MPC or in MPC Software. Play through every preset. No score replaces this step.
- **Is the DNA accurate?** A/B test key presets against similar packs in the library. If a pack is tagged CRISP and it sounds muddy, the DNA is wrong regardless of score.
- **Does the pack story hold together?** Review against the `pack_story_design` spec. The arc from first preset to last should feel intentional — not a random collection of sounds that happen to share an engine.
- **Is the naming evocative and consistent?** Read all preset names aloud. Flags: anything that sounds like a file export, anything that could belong to a different pack, anything that undersells the sound.

---

## 6. Pre-Release Checklist

1. `manifest_validator` — zero ERRORs
2. `cover_art_audit` — dimensions meet tier requirement
3. `sample_audit` — all ERRORs resolved
4. `tuning_coverage_checker` — CLEAR
5. `layer_balance_checker` — PASS
6. `kit_validator` — all kit modes intentional
7. `pad_label_optimizer` — no generic labels
8. `velocity_map_visualizer` — reviewed by a human
9. `pack_score` — meets tier threshold
10. `quick_preview_generator` — audio reviewed and approved
11. `collection_arc_validator` — PASS (collections only)
12. Ear test on physical hardware or MPC Software
13. DNA accuracy A/B test against comparable packs
14. Pack story narrative review against `pack_story_design` spec
15. `changelog_generator` + `export_report_generator` run and archived to `releases/`
