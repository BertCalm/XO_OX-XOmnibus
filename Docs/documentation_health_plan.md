# XOmnibus Documentation Health Plan
**Date:** 2026-03-20
**Scope:** Full documentation ecosystem — accuracy, governance, organization, duplication, obsolescence
**Status:** ACTIVE — Phase 1–4 defined; Phase 1–3 executed same day

---

## Executive Summary

The XOmnibus documentation ecosystem is active and well-intentioned but carries four categories of debt:
1. **Accuracy errors** — wrong counts, orphaned blessing IDs, stale debate status
2. **No governance** — no policy for "when to update which doc," no update SLAs, no metadata on skills
3. **Organizational chaos** — 162-file specs/ dumping ground, 3 duplicate guild directories, seance verdicts for original 24 engines lost
4. **Silent duplication** — engine tables maintained in 2 places, doctrines defined in 4+ places with no single designated source

---

## Phase 1 — Accuracy Fixes (EXECUTE IMMEDIATELY)

### P1.1 Coupling Types Count
**Problem:** Code has 14 coupling types (SynthEngine.h:11–37), CLAUDE.md says 13, master spec says 12.
**Root cause:** AudioToBuffer added Round 7, KnotTopology added for ORBWEAVE; neither updated in docs.
**Fix:** Update CLAUDE.md "13 coupling types" → 14. Update master spec section 4.1 to include both missing types.
**Files:** CLAUDE.md line 12, xomnibus_master_specification.md section 4.1

### P1.2 Preset Count
**Problem:** CLAUDE.md says "~15,200" but actual count is 15,542.
**Fix:** Update to "~15,500" (round number appropriate for a living codebase).
**Files:** CLAUDE.md line 14

### P1.3 New Blessings B017–B031
**Problem:** 15 new blessings confirmed/proposed in seances for OSTINATO, ORBWEAVE, OPENSKY, OUIE, OVERTONE, OCEANDEEP. All use B017+ locally causing ID collisions. CLAUDE.md still shows B001–B016 as the complete list.

**Global ID assignment (deduplicated):**

| Global ID | Name | Engine | Status |
|-----------|------|--------|--------|
| B017 | Modal Membrane Synthesis with Academic Citation | OSTINATO | Confirmed |
| B018 | Circular Topology Coupling (CIRCLE) | OSTINATO | Confirmed |
| B019 | 96 Hand-Authored World Rhythm Patterns | OSTINATO | Confirmed |
| B020 | Live Override with Graceful Yield | OSTINATO | Confirmed |
| B021 | Knot Phase Coupling Matrix | ORBWEAVE | Proposed |
| B022 | MACRO KNOT: Continuous Topology Morphing | ORBWEAVE | Proposed |
| B023 | Shepard Shimmer Architecture | OPENSKY | Confirmed |
| B024 | RISE Macro: Single-Gesture Ascension | OPENSKY | Confirmed |
| B025 | HAMMER Interaction Axis | OUIE | Confirmed |
| B026 | Interval-as-Parameter | OUIE | Confirmed |
| B027 | 8-Algorithm Palette | OUIE | Confirmed |
| B028 | Continued Fraction Convergent Synthesis | OVERTONE | Confirmed |
| B029 | Hydrostatic Compressor — Pressure as Environment | OCEANDEEP | Confirmed |
| B030 | Bioluminescent Exciter — Self-Generating Alien Texture | OCEANDEEP | Confirmed |
| B031 | Darkness Filter Ceiling — Constraint as Identity | OCEANDEEP | Confirmed |

**Fix:** Add B017–B031 table to CLAUDE.md blessings section. Update seance_cross_reference.md key blessing references to use global IDs.
**Files:** CLAUDE.md (blessings section), Docs/seance_cross_reference.md

### P1.4 Debate Resolutions
**Problem:** CLAUDE.md marks DB001–DB004 as UNRESOLVED but OSTINATO seance resolved DB003 and DB004 for percussion/rhythm engines.
**Fix:** Update debate status with seance citations.
- DB001: Expand (OSTINATO introduces behavioral chaining axis, debate unresolved at fleet level)
- DB003: RESOLVED for percussion (OSTINATO seance 2026-03-20) — immediate beauty wins
- DB004: RESOLVED for OSTINATO (expression AND evolution not in tension)
**Files:** CLAUDE.md debates section

### P1.5 Doctrines Missing from Master Spec
**Problem:** THE single source of truth has no mention of D001–D006.
**Fix:** Add "Design Doctrines" section to master spec after section 2.
**Files:** Docs/xomnibus_master_specification.md

### P1.6 Master Spec Document Hierarchy Incomplete
**Problem:** Master spec's Document Hierarchy table (section 1) lists 12 "foundation docs" but omits CLAUDE.md, seance_cross_reference, prism_sweep_final_report, xomnibus_landscape_2026, and all skills.
**Fix:** Add note distinguishing "foundation architecture docs" from "living project intelligence docs."
**Files:** Docs/xomnibus_master_specification.md

---

## Phase 2 — Governance (EXECUTE IMMEDIATELY)

### P2.1 Create Docs/GOVERNANCE.md
**Problem:** No "when to update which doc" policy exists anywhere.
**Content:** Doc update checklist per scenario (engine add, feature add, round complete), SLAs, owner assignments, doc hierarchy with authority levels.
**Files:** Docs/GOVERNANCE.md (new)

### P2.2 Create Docs/MANIFEST.md
**Problem:** No single index of all canonical documentation. The hidden `Docs/INDEX.md` exists but is not referenced anywhere.
**Content:** Full doc inventory with purpose, audience, authority level, last-updated date.
**Files:** Docs/MANIFEST.md (new), link from CLAUDE.md

### P2.3 Add Skills Metadata
**Problem:** 12 skill files have no "Last Updated," "Version," or "Status" metadata.
**Fix:** Add metadata header block to each SKILL.md: Status, Version, Last Updated, Next Review.
**Files:** All Skills/*/SKILL.md

### P2.4 Add "Updating CLAUDE.md" Section to CLAUDE.md
**Problem:** New engine process references CLAUDE.md updates but doesn't say which sections or templates.
**Fix:** Add explicit section listing the 4 places to update + template text.
**Files:** CLAUDE.md

---

## Phase 3 — Deprecation + Linking (EXECUTE IMMEDIATELY)

### P3.1 Deprecate Superseded Docs
**Problem:** xo_mega_tool_preset_system.md (82 KB) and xo_mega_tool_visual_identity.md (55 KB) are marked superseded in master spec but have no deprecation notice and could mislead new contributors.
**Fix:** Add deprecation header to both files with redirect to canonical replacement.
**Files:** Docs/xo_mega_tool_preset_system.md, Docs/xo_mega_tool_visual_identity.md

### P3.2 Link INDEX.md from CLAUDE.md
**Problem:** Docs/INDEX.md exists but is undiscoverable (not referenced anywhere).
**Fix:** Add one line to CLAUDE.md Key Files table pointing to Docs/INDEX.md.
**Files:** CLAUDE.md

### P3.3 Add concepts/ Archive Notice
**Problem:** concepts/ is historical/mythological but new contributors may treat it as current reference.
**Fix:** Update concepts/README.md header to clarify archival purpose and link to canonical sources.
**Files:** Docs/concepts/README.md

### P3.4 Add Status Headers to Docs/plans/
**Problem:** 5 plan files mix COMPLETED and ACTIVE work with no status indication.
**Fix:** Add status header to each plan file.
**Files:** All files in Docs/plans/

---

## Phase 4 — Structural Reorganization (FUTURE SPRINTS)

These are too large to execute immediately but are formally chartered here.

### P4.1 Reorganize Docs/specs/ (162 files)
**Current state:** Flat dumping ground of R&D, session notes, active specs, engine deep dives, roadmaps.
**Target structure:**
```
Docs/specs/
├── README.md           (index and organization guide)
├── active/             (current feature specs in development)
├── archive/            (completed research, superseded specs)
│   ├── prism-sweep/    (rounds 1-12 artifacts)
│   └── early-rnd/      (collections_, air_, pre-XOmnibus work)
├── engine-deep-dives/  (OBRIX, Outwit, etc. deep specs)
└── roadmaps/           (v2 strategy, feature roadmaps)
```
**Estimated effort:** 2-hour agent sprint. Move ~100 files to archive/, keep ~60 in active/.

### P4.2 Consolidate Guild Directories
**Current state:** guild/ + guild-reports/ + guild_reviews/ = 3 directories, same purpose, naming inconsistency.
**Target:** Single `Docs/guild/` with consistent naming + README explaining methodologies.
**Estimated effort:** 30 minutes.

### P4.3 Back-Generate Original 24 Seance Verdicts
**Current state:** ODDFELIX through OWLFISH seances exist only as aggregated data in xomnibus_landscape_2026.md and seance_cross_reference.md. No individual verdict files.
**Target:** 24 verdict files in Docs/seances/ reconstructed from landscape data.
**Estimated effort:** 4-hour agent sprint (24 files × 10 min each).

### P4.4 Update xomnibus_landscape_2026.md
**Current state:** Dated 2026-03-14 with pre-sweep bug counts. Bugs marked in landscape were fixed in Prism Sweep Rounds 1-12.
**Target:** Re-publish with post-sweep status, side-by-side before/after table, link to prism_sweep_final_report.md.
**Estimated effort:** 1-hour agent sprint.

### P4.5 Scripture Book Completion
**Current state:** Books I, II, and IV of scripture/the-scripture.md are placeholder headers.
**Target:** Fill Books I (Foundation Principles), II (The First Listening), and IV (Coupling Gospels) with wisdom extracted from retreats, session notes, and the 30 completed seances.
**Estimated effort:** 2-hour writing sprint.

### P4.6 Extract Anti-Patterns Guide
**Current state:** Book V (Stewardship Canons) in scripture contains 5 patterns identified empirically during development. The "Ghost Parameter Trap" and "Integration Layer Drift" patterns are referenced in code health reports but not in CLAUDE.md.
**Target:** Docs/anti_patterns_and_gotchas.md — searchable guide with code examples and citations to specific bugs.
**Estimated effort:** 1-hour extraction sprint.

---

## Known Inconsistencies Requiring Monitoring

| Issue | Current State | Owner | Check Frequency |
|-------|--------------|-------|----------------|
| Engine table in CLAUDE.md vs master spec | Synced as of 2026-03-20 | All contributors | Per engine add |
| 6D Sonic DNA definition (3 locations) | Consistent, no single crossref | Sound design team | Quarterly |
| Doctrines (5 locations) | Consistent, master spec now included | Engineering | Per doctrine change |
| knowledge/ directory | OBRIX-specific, not global | Seance team | Per seance |
| Scripture books I, II, IV | Incomplete | Writing team | Monthly |

---

## Success Metrics

- All accuracy counts verified and matching code: ✅ coupling types (14), preset count (~15,500), blessings (B001-B031)
- GOVERNANCE.md exists with clear update checklist: ✅ (Phase 2)
- MANIFEST.md exists with full doc inventory: ✅ (Phase 2)
- Skills have Last Updated metadata: ✅ (Phase 2)
- Zero files in Docs/ with no indication of active/archived status: ❌ (Phase 4 required)
- Guild directory consolidated: ❌ (Phase 4 required)
- 24 original seance verdicts preserved: ❌ (Phase 4 required)
- specs/ directory organized with subdirectories: ❌ (Phase 4 required)
