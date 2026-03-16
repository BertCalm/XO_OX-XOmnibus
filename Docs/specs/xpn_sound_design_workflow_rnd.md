# XPN Sound Design Workflow — R&D Spec
**Date**: 2026-03-16
**Audience**: XO_OX internal sound design team
**Purpose**: End-to-end process guide for creating XO_OX expansion packs, from initial concept through release prep.

---

## 1. Workflow Overview

Seven stages, sequential. Do not skip QA to save time — it compounds.

| Stage | Name | Time | Output |
|-------|------|------|--------|
| 1 | Pack Concept | 1 day | Story sentence, engine choice, DNA targets, 3 anchor names |
| 2 | Anchor Preset Design | 2–3 days | 3 anchor presets defining pack extremes + midpoint |
| 3 | Fill Preset Design | 3–5 days | Remaining preset slots navigating between anchors |
| 4 | QA Pass | 1 day | Zero ERRORs, minimal WARNINGs in qa_runner output |
| 5 | DNA Calibration | half day | Verified DNA coverage via dna_interpolator.py |
| 6 | Pack Assembly | half day | XPN manifest, cover art, oxport pipeline complete |
| 7 | Release Prep | 1 day | CHANGELOG, preview card, release_checklist.py passing |

**Total**: 9–12 working days per pack.

---

## 2. The Anchor Preset Method

Design 3 anchor presets before filling any other slots. These define the pack's creative space — everything else navigates between them.

**Anchor A — Maximum feliX Expression**
Bright, clinical, surface-level. High filter cutoff, tight envelopes, minimal reverb. This is the pack at its most alert and forward. feliX presets belong in the Foundation and Prism moods.

**Anchor B — Maximum Oscar Expression**
Warm, organic, deep. Low cutoff, slow attack, long room tail, harmonic density. This is the pack at its most immersive. Oscar presets belong in Atmosphere and Aether moods.

**Anchor C — The Coupling Sweet Spot**
Balanced, entangled. Neither pole dominates. Envelope times sit mid-range, macros actively modulate the feliX/Oscar balance. This is the pack's signature preset — the one that ends up in demos. Coupling presets belong in the Entangled mood.

All fill presets navigate the triangle formed by A, B, and C. A preset that doesn't fit somewhere on that triangle is a sign the pack concept needs tightening, not more presets.

Name your anchors first, record a 10-second audio sketch for each, and pin them at positions 01, 02, 03 in the manifest before writing a single fill preset.

---

## 3. DNA Targeting

DNA targets are design constraints, not post-hoc labels.

**Process:**
1. Write the pack's story sentence (one sentence, present tense, no adjective stacking). Example: *"Silk Road trade routes — camels, dust, slow caravans, distant minarets."*
2. Derive target ranges from the story sentence. That sentence implies: WARMTH 60–80, BRIGHTNESS 20–45, MOTION 30–60, DENSITY 40–70.
3. Before designing any preset, write down the DNA target it should hit.
4. After completing Stage 3, run `dna_interpolator.py --pack <name> --report` to check coverage. If more than 40% of presets cluster within a 15-point radius, redistribute.

DNA drift (see Common Mistakes) is almost always caused by designing presets without written targets. The interpolator catches it, but fixing it in Stage 5 is expensive — target ranges belong in your Stage 1 notes.

---

## 4. XOmnibus Preset Design Tips

**ONSET** (drum kit engine): Each preset is a full kit across 8 voices. Design the kick and snare first — they set the kit's physical character. Use the MACHINE macro to sweep between analog and digital textures before touching individual voice params. Export test kits early; the voice-to-pad mapping is where most ONSET QA failures originate.

**OPAL** (granular): Grain density above 80 significantly increases offline render time during XPN export. Keep density below 70 for packs with large preset counts unless render time is acceptable. The SCATTER macro is OPAL's personality — if M3 doesn't produce a dramatic audible shift, the preset is underdesigned.

**ORACLE** (GENDY/stochastic): ORACLE presets require human-legible breakpoint labels in the program name. When GENDY variance is above 0.6, the preset behaves differently on every note — document this in the preset description field so users aren't confused. Use the WILD macro to demonstrate the variance range, not hide it.

**OVERLAP** (FDN reverb): Feedback matrices are sensitive to small parameter changes near the stability boundary. Design OVERLAP presets by ear first, then check that the TANGLE macro sweeps from near-silence to near-feedback without crossing into runaway. Presets that clip on TANGLE max are a QA ERROR.

---

## 5. Common Mistakes

1. **DNA drift** — Presets cluster in one quadrant. Caught by dna_interpolator.py but expensive to fix late. Use target ranges from day one.
2. **Name before sound** — Program names assigned before the sound is designed lead to names that don't match the result. Design first, name last.
3. **Velocity dead zones** — Empty velocity ranges that produce silence instead of a softer dynamic layer. Test every preset at velocity 1, 64, and 127.
4. **Tuning drift across layers** — Layers within a multi-sample preset tuned to different references. Verify all layers against A440 before finalizing.
5. **Pad label collision** — Two pads sharing the same 6-character label in the XPN manifest. pack_score.py flags these; fix before assembly.
6. **Missing coupling presets** — All Entangled mood slots empty, leaving the pack without its signature feliX/Oscar balance presets. Anchor C must seed at least 3 Entangled entries.
7. **Flat macros** — M1–M4 assigned but produce no audible change, or change only a parameter value with no sonic consequence. Each macro must produce a clearly audible transformation across its full range.
8. **Release-tail clipping** — Samples with abrupt endings on looped content, creating an audible click on note release. Inspect all looped layers in an audio editor before export.

---

## 6. Tools Quick Reference

| Tool | Purpose |
|------|---------|
| `full_qa_runner.py` | Runs all QA checks across the pack; outputs ERRORs and WARNINGs |
| `pack_score.py` | Scores pack against DNA targets, mood distribution, and label integrity |
| `dna_interpolator.py` | Verifies DNA coverage and flags clustering |
| `oxport.py` | Main export pipeline — generates XPN file from manifest |
| `release_checklist.py` | Final gate check before handoff; outputs pass/fail per criterion |
| `cover_art_gen.py` | Generates pack cover art from engine accent color and story sentence |
| `manifest_builder.py` | Assembles program manifest from individual preset JSON files |
| `preset_validator.py` | Validates a single preset against engine schema |
| `velocity_map_checker.py` | Scans all velocity layers for dead zones and coverage gaps |
| `tuning_auditor.py` | Cross-checks multi-sample layer tuning against A440 reference |
| `pad_label_linter.py` | Flags duplicate or oversized pad labels in manifest |
| `mood_distributor.py` | Reports preset count per mood; flags imbalanced distributions |
| `macro_auditor.py` | Checks that all 4 macros produce parameter deltas above threshold |
| `changelog_gen.py` | Auto-generates CHANGELOG from commit messages and pack diff |
| `preview_card_gen.py` | Generates release preview card (image + spec summary) for site and Patreon |

---

*This spec is a living document. Update after each pack release with lessons learned.*
