# Community Pack Submission Pipeline — R&D Spec
**Date**: 2026-03-16 | **Status**: Draft

---

## 1. Submission Format Spec

A community contributor submits a single ZIP with this structure:

```
contributor_packname_v1/
  pack.yaml              # required — metadata manifest
  cover_art.png          # required — 1400×1400px, PNG, <2MB
  presets/               # required — one .xometa file per preset
    preset_name.xometa
    ...
  samples/               # optional — only for sample-based packs
    stems/               # dry stems only; no third-party samples
      kick_01.wav        # 44.1kHz or 48kHz, 24-bit, mono or stereo
      ...
  LICENSE.txt            # required — contributor signature + grant
```

**`pack.yaml` required fields:**
```yaml
pack_name: "Submerged"
contributor_handle: "beatmaker_x"
contributor_email: "contact@example.com"
target_engines: [OPAL, DRIFT]   # XO_OX engine names
mood_tags: [Atmosphere, Flux]
bpm_range: [85, 130]            # optional for tonal packs
xo_ox_version: "1.0"           # XOlokun version targeted
```

Cover art: 1400×1400px minimum, PNG or JPG, no embedded ICC profiles, no third-party logos, no AI-generated art (declared via pack.yaml `art_origin: original`).

---

## 2. Automated QA Gates

Run sequentially via `xpn_community_qa.py` (new tool — see Section 7):

| Gate | Tool | Pass Criteria |
|------|------|---------------|
| Schema validation | `xpn_validator.py` | All .xometa files parse without error |
| Engine compatibility | `xpn_validator.py --engine-check` | Param IDs match declared target engines |
| Sample integrity | `xpn_qa_checker.py` | No clipped samples (peak > -0.3 dBFS fails), no zero-length files |
| Level normalization | `xpn_qa_checker.py --lufs` | Integrated LUFS between -18 and -10; flag outliers, do not auto-reject |
| Sonic DNA presence | `audit_sonic_dna.py` | Every .xometa has non-null DNA field |
| Cover art check | `xpn_submission_packager.py --art-check` | Dimensions ≥ 1400px, file size < 2MB |
| Duplicate detection | `xpn_submission_packager.py --dedup` | SHA-256 of each sample vs. existing XO_OX sample library |

If any gate hard-fails (schema, engine compat, zero-length files), the submission is auto-rejected with a structured report emailed back to the contributor. LUFS outliers and missing DNA generate warnings that go to human review.

---

## 3. Human Review Checklist

After automated QA passes, a curator runs through this checklist (target: 15 minutes per submission):

1. **Sonic character** — Does the pack have a distinct voice? Does it align with at least one XO_OX mood axis? Reject generic preset dumps.
2. **Preset naming quality** — Names should be evocative, not generic (`Pad 01` fails). Spot-check 5 presets.
3. **Sample clearance declaration** — Confirm LICENSE.txt is signed and `art_origin` field is present. Flag any sample names that suggest third-party sources (e.g. `rhodes_xxx.wav`).
4. **Engine coupling integrity** — If presets reference coupling targets (e.g. OPAL→DUB), verify the coupling params exist in the .xometa and make sense musically.
5. **Cover art vibe check** — Does it fit XO_OX visual language? No drop shadows, no stock photo collages. Should feel like it belongs in the Aquarium.
6. **Playability pass** — Load 3–5 presets in XOlokun and play them. Do they respond musically to velocity and mod wheel?
7. **Pack cohesion** — Do the presets belong together? A pack should feel like a world, not a random collection.

Curator leaves inline comments in a review YAML file and either approves, requests revision (with specific notes), or rejects.

---

## 4. Tiered Credit System

| Tier | Criteria | Reward |
|------|----------|--------|
| **Contributor** | 1 accepted pack | Free download credits (equivalent to 2 packs), Discord role: `XO Contributor` |
| **Featured** | 3+ accepted packs or curator pick | Co-branded pack listing ("feat. [handle]"), Patreon tier upgrade (1 level) |
| **Resident** | 6+ packs or invited artist | Named artist page on XO-OX.org, revenue share on pack sales (15%), Discord role: `XO Resident` |

All contributors get permanent credit in pack metadata and on the XO-OX.org pack listing page. Credit is non-revocable once a pack ships.

---

## 5. Technical Pipeline (Year 1: 5–20 submissions/month)

Keep it simple. No custom platform needed at this scale.

**Intake**: Google Form collects contributor info + Google Drive upload link. Contributor uploads their ZIP to a shared Drive folder. Form response triggers a Zapier/Make webhook.

**Automated QA**: Webhook calls a lightweight Python script (`xpn_community_qa.py`) running on a small VPS or GitHub Actions cron. Script pulls the ZIP from Drive, runs all QA gates, writes a `qa_report.yaml`, and posts results to a private Discord channel (`#pack-submissions`).

**Human review**: Curator picks up submissions flagged as QA-passed in Discord. Reviews using the checklist above. Decision recorded in a shared Airtable (pack name, status, curator notes, decision date).

**Acceptance**: On approval, `xpn_submission_packager.py` repackages the submission into XO_OX standard bundle format, generates final cover art crop variants, and outputs a deploy-ready ZIP. Curator commits to the packs repo; CI publishes to XO-OX.org.

**Rejection/revision**: Curator posts structured feedback back to contributor via email (templated). Contributor may resubmit with a new version tag.

No GitHub PR workflow for contributors — too high a barrier. PRs remain internal (XO_OX team only).

---

## 6. Legal / Licensing

**What contributors grant XO_OX:**
A non-exclusive, royalty-free, worldwide license to distribute, host, sell, and promote the submitted pack as part of XO_OX products and services. XO_OX may bundle the pack, feature it in promotional materials, and include it in subscription tiers.

**What contributors retain:**
Full ownership of their original samples, preset designs, and creative work. They may continue to distribute their work independently, provided they do not misrepresent it as an exclusive XO_OX product.

**What contributors warrant:**
All submitted samples are original recordings or cleared for commercial use. No third-party copyrighted material. AI-generated samples must be declared; XO_OX reserves the right to decline them.

Include a one-sentence sign-off in LICENSE.txt: *"I grant XO_OX a non-exclusive license to distribute this pack and confirm all included material is original or cleared for commercial use."*

---

## 7. New Oxport Tools to Build (Sonnet-Ready)

Three targeted tools that fill gaps in the current Oxport suite:

**`xpn_submission_packager.py`**
Ingests a raw contributor ZIP, validates structure against the submission spec, renames/normalizes file paths to XO_OX conventions, generates cover art crop variants (1400×1400, 600×600, 300×300), and outputs a clean `submission_ready.zip`. Wraps existing `xpn_validator.py` and `xpn_cover_art.py`. ~200 lines.

**`xpn_community_qa.py`**
Orchestrates the full automated QA pipeline: runs all gates in sequence, collects results, writes `qa_report.yaml` with pass/warn/fail per gate plus per-file details. Designed to run headless in GitHub Actions or a cron job. Outputs a human-readable summary for Discord posting. ~300 lines.

**`xpn_dedup_checker.py`**
Builds and maintains a SHA-256 fingerprint index of all samples in the XO_OX library. On submission, checks each incoming sample against the index and flags exact duplicates. Also does a fuzzy check via short MD5 of the first 4096 bytes to catch re-exported copies of the same source. Outputs a `dedup_report.yaml`. ~150 lines.

All three tools follow the existing Oxport pattern: CLI entry point, YAML config optional, structured output for downstream consumption.
