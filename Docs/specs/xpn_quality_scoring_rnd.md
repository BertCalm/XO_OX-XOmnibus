# XPN Pack Quality Scoring Rubric — R&D Spec
**Version**: 1.0 | **Date**: 2026-03-16 | **Status**: Active Standard

All XO_OX XPN packs are scored against this 100-point rubric before release. A pack that does not reach the PASS threshold ships nothing.

---

## Release Gates

| Score | Gate | Meaning |
|---|---|---|
| 85–100 | **PASS** | Clear for release |
| 70–84 | **CONDITIONAL PASS** | Fix flagged items before wide release; soft launch to Patreon only |
| 0–69 | **FAIL** | Return to development; do not release |

---

## Category 1: Technical Quality — 25 points

### Sample Rate Compliance (5 pts)
**Measurement**: Automated. Inspect all `.wav` headers in the pack. Every file must report 44100 Hz or 48000 Hz.
**Pass**: All files compliant — 5 pts. Any file outside these rates — 0 pts.
**Fix**: Resample non-compliant files using SoX or Reaper render at the source engine's native rate. Never upsample from a lower rate; re-render from the engine.

### Bit Depth Compliance (5 pts)
**Measurement**: Automated. Every `.wav` must be 16-bit or 24-bit integer PCM. No 32-bit float exports in the release bundle.
**Pass**: All files compliant — 5 pts. Any violation — 0 pts.
**Fix**: Re-export from the XPN Tools pipeline with `--bit-depth 24`. Floating-point exports are a pipeline misconfiguration.

### No Clipping (5 pts)
**Measurement**: Automated. Run peak analysis across all files; flag any file with a peak at or above -0.5 dBFS.
**Pass**: Zero files clipping — 5 pts. 1–2 files — 2 pts. 3+ files — 0 pts.
**Fix**: Lower render gain at source. Do not apply a limiter to mask clipping — fix the engine output level.

### Consistent Levels Across Kit (5 pts)
**Measurement**: Automated. Compute RMS for each sample at the loudest velocity layer. The spread across all 16 pads must not exceed ±6 dB RMS.
**Pass**: Within ±6 dB — 5 pts. ±6–9 dB — 2 pts. Beyond ±9 dB — 0 pts.
**Fix**: Per-voice gain staging inside the engine or in the XPN Tools render spec. Kick and snare naturally sit louder — target perceptual balance, not flat RMS.

### No Silent or Dead Samples (5 pts)
**Measurement**: Automated. Any file with RMS below -60 dBFS across its full duration is flagged as dead.
**Pass**: Zero dead files — 5 pts. Any dead file — 0 pts.
**Fix**: Re-render, or replace with a rest marker. Never ship a placeholder silence.

---

## Category 2: Playability — 25 points

### Velocity Response Is Musically Useful (10 pts)
**Measurement**: Human review. Play each pad through the full velocity range (pp, mp, mf, ff). Velocity must produce audible, musical change — timbre shift, level arc, or both. Vibe's musical velocity curve is the reference standard.
**Pass**: All pads respond musically — 10 pts. Minor dead zones on 1–2 pads — 6 pts. Flat velocity on 3+ pads — 0 pts.
**Fix**: Revisit velocity layer boundaries and crossfade widths in the XPN Tools kit config.

### All 16 Pads Respond Meaningfully (5 pts)
**Measurement**: Human review. Every assigned pad triggers a distinct, intentional sound. Duplicate assignments are only acceptable if intentional (e.g., velocity-split doubles).
**Pass**: All 16 responsive and intentional — 5 pts. 1–2 pads weak/duplicate — 2 pts. 3+ — 0 pts.
**Fix**: Redesign pad assignments. A half-empty kit is a half-finished pack.

### Loop Points Clean (5 pts)
**Measurement**: Human review. Applies only to packs with looped samples. Audition loop start/end points at slow tempo. Zero audible clicks or thumps.
**Pass**: All loops clean — 5 pts. One click — 2 pts. Two or more — 0 pts. (N/A = full 5 pts for one-shot-only packs.)
**Fix**: Zero-cross the loop endpoints. Add a short crossfade (2–5 ms) if waveform alignment is impractical.

### One-Shot Samples Decay Naturally (5 pts)
**Measurement**: Human review. One-shots must tail to silence without abrupt truncation or unnatural ring. The final 10% of the file should be envelope decay, not a hard cut.
**Pass**: All one-shots decay cleanly — 5 pts. 1–2 truncated — 2 pts. 3+ — 0 pts.
**Fix**: Extend the engine's amplitude envelope release, or pad the render with silence tail in the spec.

---

## Category 3: Sonic Identity — 20 points

### Clear feliX-Oscar Placement (5 pts)
**Measurement**: Human review by at least one XO_OX team member who has not worked on the pack. The overall character should land unambiguously on the feliX-Oscar spectrum — or document the deliberate tension.
**Pass**: Placement clear and documented — 5 pts. Ambiguous but defensible — 2 pts. No placement — 0 pts.
**Fix**: Define the placement in `pack.yaml` under `felix_oscar_axis` and tune the macro defaults to reinforce it.

### Consistent Character Across Kit (5 pts)
**Measurement**: Human review. Play the full kit as a groove. All pads should sound like they belong to the same world — same space, same aesthetic logic.
**Pass**: Coherent kit — 5 pts. 1–2 pads feel foreign — 2 pts. Kit is incoherent — 0 pts.
**Fix**: Unify reverb tail length, tonal register, or processing chain across voices.

### DNA Tags Accurate (5 pts)
**Measurement**: Human review cross-referenced against the XPN Tools DNA tag vocabulary. Every tag applied must be audibly present in the samples.
**Pass**: All tags justified — 5 pts. 1 questionable tag — 3 pts. 2+ unjustified tags — 0 pts.
**Fix**: Remove aspirational tags. Tag what is audible, not what was intended.

### Engine Character Preserved in Renders (5 pts)
**Measurement**: Human review. Compare pack samples against the live engine playing identical parameters. The renders must capture the engine's signature — not a thinned, over-processed version.
**Pass**: Renders faithful to live engine — 5 pts. Minor loss — 2 pts. Renders unrecognizable — 0 pts.
**Fix**: Re-examine the render chain for excessive post-processing. The engine is the instrument; renders are recordings of it.

---

## Category 4: Documentation — 15 points

### pack.yaml Complete (5 pts)
**Measurement**: Automated schema validation. Required fields: `name`, `engine`, `version`, `felix_oscar_axis`, `mood_tags`, `dna_tags`, `coupling_suggestions`, `preset_count`.
**Pass**: All required fields present and non-empty — 5 pts. 1–2 missing — 2 pts. 3+ missing — 0 pts.
**Fix**: Fill the schema. No undocumented releases.

### Cover Art Present and On-Brand (5 pts)
**Measurement**: Human review. A 1000×1000 px minimum PNG must be present. Art must use the engine's accent color and adhere to XO_OX visual identity (no stock photo collage, no generic presets screenshots).
**Pass**: Art present and on-brand — 5 pts. Art present but off-brand — 2 pts. No art — 0 pts.
**Fix**: Commission or generate art referencing the engine's aquatic identity card.

### MPCE_SETUP.md or README Present (5 pts)
**Measurement**: Automated. For MPCe packs: `MPCE_SETUP.md` required. For all other pack types: `README.md` required. File must be non-empty (>100 words).
**Pass**: File present and substantive — 5 pts. File present but skeletal — 2 pts. Missing — 0 pts.
**Fix**: Write the setup doc. One paragraph minimum: what the pack is, how to load it, one production tip.

---

## Category 5: Uniqueness — 15 points

### Not Duplicating Existing XO_OX Packs (5 pts)
**Measurement**: Human review against the full XO_OX catalog. The new pack must offer a meaningfully distinct tonal or rhythmic territory from any existing release.
**Pass**: Clearly distinct — 5 pts. Adjacent but defensibly different — 2 pts. Duplicate — 0 pts.
**Fix**: Identify the differentiating engine macro position or performance character and amplify it.

### At Least 1 Signature Moment Preset (5 pts)
**Measurement**: Human review. One preset must be immediately striking — the kind of sound that stops a producer mid-scroll. It should be demo-ready on its own.
**Pass**: Signature moment present — 5 pts. Pack is solid but no standout — 2 pts. Nothing memorable — 0 pts.
**Fix**: Spend design time specifically hunting for the unexpected. Push macros to extremes; render the accident.

### Coupling Documentation Present (5 pts)
**Measurement**: Automated + human. `pack.yaml` must list at least one `coupling_suggestions` entry. The suggested coupling engine must exist in XOlokun.
**Pass**: At least one valid coupling documented — 5 pts. Field present but empty — 0 pts.
**Fix**: Identify the natural feliX→Oscar or Oscar→feliX pair for this pack and document it. Every pack has a natural counterpart.

---

## Scoring Summary

| Category | Points |
|---|---|
| Technical Quality | /25 |
| Playability | /25 |
| Sonic Identity | /20 |
| Documentation | /15 |
| Uniqueness | /15 |
| **Total** | **/100** |

A CONDITIONAL PASS score (70–84) must identify every failing sub-criterion by name, assign an owner, and set a fix deadline before the soft launch date. No conditional pass item may remain open at wide release.

This rubric is the floor. It measures the minimum. A 100-point pack is not necessarily great — it is simply ready to ship.
