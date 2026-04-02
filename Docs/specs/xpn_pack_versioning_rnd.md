# XPN Pack Version Management — R&D Spec
**Status**: Draft | **Date**: 2026-03-16 | **Author**: XO_OX R&D

---

## Overview

XO_OX XPN packs are living products. They receive bugfixes, receive new content, and occasionally undergo structural changes that require MPC users to update their projects. Without a clear versioning contract, users cannot know what changed, tools cannot enforce compatibility, and the fleet becomes impossible to audit.

This spec defines the versioning system, release protocols, file format requirements, and tooling support for all XPN packs shipped by XO_OX.

---

## 1. Semver for Packs

All packs use `MAJOR.MINOR.PATCH` semantic versioning.

### 1.1 PATCH (e.g., 1.0.1)

Bugfixes only. No new programs, no new samples, no renamed programs.

Examples:
- Corrected a wrong choke group assignment on CHat programs
- Replaced a clipping sample with a normalized version
- Fixed a malformed `expansion.json` field
- Corrected PadNoteMap/PadGroupMap index errors
- Fixed Q-Link assignment that pointed to wrong parameter

PATCH updates are safe. Existing MPC projects reference the same programs by the same names and they continue to work — they just sound correct now.

### 1.2 MINOR (e.g., 1.1.0)

Additive changes. New programs, new samples, updated DNA values, new categories.

Examples:
- Added 4 new programs (e.g., `IRON_KICK_Garage`)
- Added a bonus velocity layer to existing programs
- Revised DNA values to better match the engine's sonic character
- Added new kit configurations (e.g., a Minimal variant)

**Rule: never remove or rename programs in a MINOR update.** MPC project files reference programs by name. Renaming or removing a program in a MINOR update silently breaks existing user projects. If a rename is necessary, it must be deferred to a MAJOR update — and the CHANGELOG must document the old and new names explicitly.

### 1.3 MAJOR (e.g., 2.0.0)

Breaking changes. Program names changed, programs removed, fundamental kit structure reorganized.

Examples:
- Renamed `IRON_KICK_Deep` → `IRON_KICK_Sub` for naming consistency
- Removed a set of prototype programs that shipped by mistake
- Restructured the program hierarchy (e.g., consolidated 3 kits into 1)
- Swapped sample pool to a new recording session (all buffers replaced)

MAJOR updates must include a migration note in the CHANGELOG explaining what broke and what users need to do to update existing MPC projects.

### 1.4 Pre-release Identifiers

Packs must not ship as `1.0.0` until they have passed the full pre-launch checklist (see Section 7).

Pre-release tags:
- `1.0.0-beta.1` — feature-complete, under sound design review
- `1.0.0-rc.1` — all checklist items resolved, final sign-off pending
- `1.0.0` — shipped

Beta and RC versions may be distributed to testers but must not appear on Gumroad as public products.

---

## 2. Gumroad Version Update Protocol

When releasing a new version of an existing pack:

1. **Update the existing Gumroad product** — do not create a new listing. Gumroad automatically notifies all previous purchasers of the updated file.
2. Upload the new ZIP (with updated filename, e.g., `ONSET_IronMachines_v1.1.xpn`).
3. Update the product description to reflect the new version number and summarize what changed.
4. Include a `CHANGELOG.md` in the ZIP root (see Section 3).
5. Update `pack_registry.json` at repo root (see Section 6).
6. Tag the release commit: `git tag packs/ONSET_IronMachines/v1.1.0`.

Do not create a separate Gumroad product for v2 unless the pack has been so thoroughly restructured that it represents a distinct product. In that case, keep the v1 listing active (existing purchasers retain access) and link to v2 from the v1 description.

---

## 3. CHANGELOG.md Format

Each pack ZIP must include a `CHANGELOG.md` at the ZIP root. Format:

```markdown
# ONSET — Iron Machines CHANGELOG

## v1.1.0 — 2026-06-01
- Added 4 new programs: IRON_KICK_Garage, IRON_SNARE_Trap, IRON_CHAT_Tight, IRON_CLAP_Smack
- Fixed choke group assignments on all CHat programs (were assigned to group 2, now correctly group 1)
- Updated Sonic DNA velocity mapping for IRON_KICK_Sub — now matches ONSET engine default curve

## v1.0.0 — 2026-04-01
- Initial release
- 12 programs, 3 kit configurations (Full, Minimal, Performance)
- 64 samples across Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX voices
```

Rules:
- Most recent version at the top.
- Date in ISO 8601 format (`YYYY-MM-DD`).
- Each bullet describes one discrete change — avoid vague entries like "various improvements."
- MAJOR updates must include a `### Migration` subsection listing renamed/removed programs.

---

## 4. Version Field Propagation

The version string must appear consistently in four locations. Tools must keep these in sync — a mismatch is a build error.

### 4.1 `expansion.json`

```json
{
  "name": "Iron Machines",
  "engine": "ONSET",
  "version": "1.1.0",
  "min_mpc_os": "3.3.0"
}
```

### 4.2 `bundle_manifest.json`

```json
{
  "pack_name": "Iron Machines",
  "engine": "ONSET",
  "version": "1.1.0",
  "min_mpc_os": "3.3.0",
  "release_date": "2026-06-01",
  "programs": [...]
}
```

### 4.3 XPM `<Program>` attribute

Each `.xpm` file includes a `version` attribute on the root `<Program>` element:

```xml
<Program version="1.1.0" name="IRON_KICK_Deep" type="Drum">
```

This allows the MPC to surface version information per-program if Akai ever exposes it in the UI, and makes forensic debugging possible when a user sends a broken project file.

### 4.4 ZIP filename

```
ONSET_IronMachines_v1.1.xpn
```

Convention: `{ENGINE}_{PackName}_v{MAJOR}.{MINOR}.xpn` — PATCH is omitted from the filename to avoid confusing users with micro-updates. PATCH is still tracked internally in the manifest and CHANGELOG.

### 4.5 Sync enforcement

The `xpn_packager.py` tool must validate that `expansion.json`, `bundle_manifest.json`, and a sample of XPM files all report the same version before writing the ZIP. Any mismatch aborts the build with a clear error message.

---

## 5. Compatibility Contract — `min_mpc_os`

Not all XPN features are supported on all MPC OS versions. Packs that use features introduced in a specific OS version must declare a minimum requirement.

### 5.1 `min_mpc_os` field

Add `min_mpc_os` to `bundle_manifest.json` and `expansion.json` (see Section 4). Value is a semver string matching Akai's MPC OS versioning (e.g., `"3.3.0"`).

### 5.2 Known compatibility breakpoints

| MPC OS | Feature introduced |
|--------|--------------------|
| 3.0.0  | XPN expansion format baseline |
| 3.2.0  | CycleType round-robin (`random-norepeat`) |
| 3.3.0  | Q-Link assignments in XPM |
| 3.4.0  | Per-pad color assignments |

This table lives in `Tools/docs/mpc_os_compatibility.md` and is updated as new OS features are used in packs.

### 5.3 Tooling behavior

If `xpn_packager.py` detects that a pack uses a feature associated with a minimum OS version higher than the declared `min_mpc_os`, it emits a warning and prompts the author to update the field. It does not auto-correct — the author must confirm.

---

## 6. Version Tracking Across the Fleet — `pack_registry.json`

A single file at repo root tracks the entire pack fleet.

**Location**: `~/Documents/GitHub/XO_OX-XOceanus/pack_registry.json`

### 6.1 Schema

```json
{
  "registry_version": "1.0.0",
  "last_updated": "2026-03-16",
  "packs": [
    {
      "pack_id": "onset_iron_machines",
      "display_name": "Iron Machines",
      "engine": "ONSET",
      "current_version": "1.0.0",
      "release_date": "2026-04-01",
      "status": "released",
      "gumroad_product_id": "abcde",
      "min_mpc_os": "3.3.0",
      "zip_filename": "ONSET_IronMachines_v1.0.xpn",
      "changelog_path": "Packs/ONSET/IronMachines/CHANGELOG.md"
    }
  ]
}
```

### 6.2 Status values

| Status | Meaning |
|--------|---------|
| `concept` | Pack idea, no programs yet |
| `beta` | Under development, not yet on Gumroad |
| `rc` | Release candidate, final review |
| `released` | Live on Gumroad |
| `deprecated` | Superseded by a major revision; old listing kept for existing purchasers |

### 6.3 `xpn_pack_analytics.py` integration

The analytics tool reads `pack_registry.json` to:
- List all released packs with current versions
- Flag packs where `current_version` in registry does not match the version in the corresponding `bundle_manifest.json` (desync warning)
- Report time-since-last-update per pack (staleness audit)
- Generate a release notes summary across the fleet for a given date range

Usage:
```bash
python Tools/xpn_pack_analytics.py --fleet-status
python Tools/xpn_pack_analytics.py --stale --days 180
python Tools/xpn_pack_analytics.py --release-notes --since 2026-01-01
```

---

## 7. The v1.0.0 Release Gate

A pack must not be tagged `1.0.0` until all items on this checklist are resolved. Failing any item is a release blocker.

### Pre-launch checklist

**Content**
- [ ] All programs load without errors on MPC (hardware or software)
- [ ] No clipping samples — all peaks confirmed below -0.3 dBFS
- [ ] All choke groups verified correct
- [ ] PadNoteMap/PadGroupMap assignments verified against intended kit layout
- [ ] Q-Link assignments confirmed functional

**Metadata**
- [ ] `expansion.json` version field set to `1.0.0`
- [ ] `bundle_manifest.json` version field set to `1.0.0`
- [ ] All XPM files include `version="1.0.0"` attribute
- [ ] `min_mpc_os` field set and verified against features used
- [ ] CHANGELOG.md present at ZIP root with `## v1.0.0` entry

**Sound design**
- [ ] Sonic DNA values reviewed and approved
- [ ] Guru Bin review (soul preservation) passed
- [ ] Preset listening pass complete — no obvious duds in the program list

**Packaging**
- [ ] ZIP filename matches convention: `{ENGINE}_{PackName}_v1.0.xpn`
- [ ] `xpn_packager.py` version sync check passes (no mismatches)
- [ ] `pack_registry.json` entry created with status `released`
- [ ] Gumroad product created with correct description and cover art

**Documentation**
- [ ] Pack listed in relevant XO-OX.org packs page
- [ ] Pack entry added to Aquarium depth zone if applicable

### Release commit convention

```
git tag packs/ONSET_IronMachines/v1.0.0
git push origin packs/ONSET_IronMachines/v1.0.0
```

---

## 8. Open Questions

- **Downgrade policy**: If a v1.1.0 pack introduces a regression, can we ship v1.1.1 that rolls back content added in 1.1.0? Technically yes under PATCH rules (it's a bugfix). But removing programs added in 1.1.0 violates the MINOR non-removal rule. Resolution: a rollback of content is treated as a MAJOR bump (2.0.0), even if the intent is corrective. Communicate clearly in CHANGELOG.
- **MPC OS version detection**: The MPC has no mechanism to warn users loading a pack that requires a newer OS. This is a platform limitation. Mitigation: state `min_mpc_os` prominently in the Gumroad product description.
- **Automated version bump tooling**: A `bump_version.py` script that updates all four version locations atomically would reduce human error. Spec for that tool is deferred to a follow-on R&D note.

---

## Summary

| Concern | Decision |
|---------|----------|
| Version format | `MAJOR.MINOR.PATCH` semver |
| Never break | Program names — only remove/rename in MAJOR |
| Distribution | Update existing Gumroad product, never create new listing |
| Changelog | `CHANGELOG.md` in ZIP root, reverse-chronological |
| Version locations | `expansion.json`, `bundle_manifest.json`, XPM attribute, ZIP filename |
| OS compatibility | `min_mpc_os` field in manifests |
| Fleet tracking | `pack_registry.json` at repo root |
| Release gate | Full checklist required before `1.0.0` tag |
| Pre-release | `1.0.0-beta.1` → `1.0.0-rc.1` → `1.0.0` |
