# XPN Pack Pre-Launch Checklist

**Version**: 1.0 | **Date**: 2026-03-16
**Purpose**: The definitive gate before any XO_OX pack goes live on Gumroad or XO-OX.org.
Every checkbox must be resolved before proceeding to distribution. Use this file as a PR template
or a manual sign-off sheet — no item ships amber.

---

## 1. Technical QA
*Items in this section can be fully or partially automated via `xpn_pack_score.py`.*

### XML & Format Integrity
- [ ] All `.xpm` files pass XML schema validation (no malformed tags, unclosed elements)
- [ ] All `.xpm` files load without error on MPC firmware 3.x and MPCE
- [ ] `expansion.json` present at pack root and is valid JSON (run `python -m json.tool`)
- [ ] `bundle_manifest.json` present with all required fields: `name`, `version`, `engine`, `tier`, `program_count`, `sample_count`, `author`, `release_date`
- [ ] No `VelStart=0` on non-empty layers — ghost trigger bug; every non-silent layer must have `VelStart ≥ 1`
- [ ] `KeyTrack=True` on all keygroup programs (pitch follows keyboard; omitting this = all notes play at root pitch)
- [ ] `RootNote=0` on all keygroup programs unless a deliberate transposition is intended and documented
- [ ] Choke groups correctly assigned for all drum programs:
  - [ ] Closed hi-hat layers: `Choke Group = 1`
  - [ ] Open hi-hat layers: `Choke Group = 1` (same group so closed kills open)
  - [ ] Snare variations: `Choke Group = 2`
  - [ ] Clap variations: `Choke Group = 3`
  - [ ] No choke group collisions between unrelated sounds
- [ ] `CycleType` values are valid: `Velocity`, `Cycle`, `Random`, `Random-NoRepeat`, or `Smart`
- [ ] `PadNoteMap` entries do not exceed 128 (MPC silently drops out-of-range pads)
- [ ] Q-Link assignments present and map to at least 4 musically useful params per program

### File & Asset Integrity
- [ ] Cover art present as `cover.png` or `cover.jpg` at pack root
- [ ] Cover art dimensions ≥ 400 × 400 px; square aspect ratio (1:1)
- [ ] Cover art ≤ 2MB (Gumroad thumbnail upload limit)
- [ ] All sample files referenced in `.xpm` exist at the declared relative paths (no broken links)
- [ ] All sample files are 44.1 kHz or 48 kHz WAV (no MP3, no AIFF, no 32kHz legacy files)
- [ ] SHA-256 hash check: no two samples in the pack share an identical hash (no accidental duplicates)
- [ ] Total uncompressed pack size < 500 MB
- [ ] ZIP archive builds cleanly with `xpn_packager.py` and opens without error on macOS and Windows

### Metadata Consistency
- [ ] Pack `version` in `expansion.json` matches `bundle_manifest.json` and README front-matter
- [ ] Engine name in manifest matches the canonical engine name in the XOmnibus engine table
- [ ] Tier field is one of: `SIGNAL`, `FORM`, or `DOCTRINE`
- [ ] `author` field is `XO_OX` (not a personal name or placeholder)
- [ ] No placeholder strings (`TODO`, `TBD`, `FIXME`) anywhere in JSON or XPM files

---

## 2. Sound Design QA
*Human ears required. No automation can substitute. Minimum one full listening pass per item.*

### Playability
- [ ] Every preset sounds compelling dry — no effects chain masking a weak core sound
- [ ] Every preset sounds acceptable at 127 velocity and at 64 velocity (no dramatic unintended change)
- [ ] Velocity response is audible on all programs: soft hits should feel meaningfully quieter/softer than hard hits
- [ ] Macro / Q-Link knobs produce an audible, musically useful change across the full 0–127 range
- [ ] No preset clips or distorts at full velocity (127); peak output ≤ −1 dBFS
- [ ] No preset is silent at any velocity (check low-velocity layers for accidental VelEnd cutoff)
- [ ] Choke groups function correctly in live playback: closed hat kills open hat ring cleanly

### Character & Spread
- [ ] feliX-Oscar polarity is represented: at least 20% of presets lean bright/energetic (feliX), at least 20% lean dark/atmospheric (Oscar)
- [ ] No two presets are perceptually identical — each earns its place in the list
- [ ] The pack has a coherent sonic identity: a first-time user could describe the pack's "personality" in one sentence

### Naming
- [ ] All preset names are unique within the pack
- [ ] All preset names are ≤ 30 characters (MPC display truncates beyond this)
- [ ] No jargon, abbreviations, or internal codenames in preset names (user-facing only)
- [ ] Naming style is consistent: no mix of ALL_CAPS and Title Case in the same pack

---

## 3. Documentation

### In-Pack Files
- [ ] `README.md` present at pack root
- [ ] README contains: pack name, version, engine(s) required, tier, program count, sample count, author, release date
- [ ] README contains a `## Quick Start` section (load → hear → tweak in under 5 minutes)
- [ ] README contains a `## Credits` section
- [ ] `MPCE_SETUP.md` present with quad-corner pad assignments (pads 1/4/13/16 = performance defaults)
- [ ] `MPCE_SETUP.md` includes Q-Link macro descriptions for each program type in the pack

### Product Metadata
- [ ] Pack tier clearly stated: `SIGNAL` (free/intro), `FORM` (mid), or `DOCTRINE` (flagship)
- [ ] Engine compatibility listed: which XOmnibus engines the pack was designed for
- [ ] Minimum firmware version stated if the pack uses MPC 3.x-only features
- [ ] Version number follows semantic convention: `MAJOR.MINOR.PATCH` (e.g., `1.0.0` for first release)
- [ ] Changelog present if this is an update to a previously released pack

---

## 4. Distribution

### Gumroad
- [ ] Gumroad product page created (not left as draft)
- [ ] Product description written: 150–300 words, covers identity/character, not just spec list
- [ ] Cover art uploaded to Gumroad product page (1:1, ≥ 400px)
- [ ] Preview audio or demo video attached (minimum: 60-second audio clip)
- [ ] Price set per tier guidelines:
  - `SIGNAL`: free or pay-what-you-want (minimum $0)
  - `FORM`: $9–$19
  - `DOCTRINE`: $24–$49
- [ ] Tags/categories set on Gumroad (MPC, XPN, Sampler, [engine name], XO_OX)
- [ ] Download file is the `.xpn` ZIP, not a raw folder

### XO-OX.org
- [ ] XO-OX.org `/packs` page updated with new pack entry
- [ ] Pack card includes: name, tier badge, engine label, cover art, Gumroad link
- [ ] If pack is tied to a Field Guide post, the post links to the pack and vice versa
- [ ] Aquarium depth zone updated if pack introduces a new engine or expands an existing engine's content

### Announcements
- [ ] Twitter/X copy drafted (≤ 280 chars, includes pack name, tier, price, link)
- [ ] Instagram caption drafted (3–5 sentences + hashtag block)
- [ ] Patreon post drafted for soft-launch window (see protocol below)

---

## 5. Soft Launch vs. Full Launch Protocol

XO_OX uses a **72-hour Patreon-exclusive window** before every pack goes public. This rewards
early supporters, generates first-wave feedback, and creates organic word-of-mouth.

### Soft Launch (Hours 0–72): Patreon Only

- [ ] Gumroad product page is **set to "Patreon-only" or unlisted** — direct link shared via Patreon post only
- [ ] Patreon post published with: pack story, what inspired it, download link, request for feedback
- [ ] Post category: "Exclusive Drop" | Tier access: All paid tiers (SIGNAL supporters and above)
- [ ] Monitor for feedback: fix any show-stopping bugs reported within the 72-hour window before public release
- [ ] If a critical bug is found in soft launch, pull the link, fix, re-upload, and reset the 72-hour clock

### Full Launch (Hour 72+): Public Release

- [ ] Gumroad product page switched to **public / listed**
- [ ] Patreon post updated: "Now live for everyone — link in bio"
- [ ] Twitter/X announcement posted
- [ ] Instagram post published
- [ ] XO-OX.org `/packs` page entry goes live (if held behind a draft toggle)
- [ ] Link added to XO-OX.org homepage "New Releases" strip if applicable
- [ ] Post in any relevant communities (MPC forums, Splice Discord, etc.) — organic only, no spam

### Soft Launch Exemptions
A pack may skip the 72-hour window only in these cases:
- Free / SIGNAL tier pack with no purchase risk to buyers
- Hotfix update to a previously released pack (bug correction, not new content)
- Time-sensitive collab or seasonal drop with a hard external deadline

Document the exemption reason in `bundle_manifest.json` under the key `"soft_launch_skipped_reason"`.

---

## Sign-Off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Sound Design | | | |
| Technical QA | | | |
| Distribution | | | |

> All three sign-offs required before a DOCTRINE tier pack ships.
> SIGNAL and FORM packs require Technical QA + Distribution sign-off minimum.

---

*Maintained by XO_OX. Update this checklist whenever a new XPN format feature is adopted
or a new distribution channel is added.*
