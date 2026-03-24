# XOmnibus Documentation Manifest
**Last Updated:** 2026-03-23
**Purpose:** Canonical inventory of all major documentation — purpose, audience, authority level, and currency status.

For governance rules (when to update what), see `GOVERNANCE.md`.
For the detailed documentation improvement plan, see `documentation_health_plan.md`.

---

## Level 1 — Canonical (Must Stay Current)

| Document | Purpose | Audience | Authority For | Status |
|----------|---------|----------|---------------|--------|
| `../CLAUDE.md` | Master project guide for Claude agents and contributors | Agents, all contributors | Engine registry, blessings, debates, doctrines, architecture rules | ✅ Current (2026-03-20) |
| `xomnibus_master_specification.md` | Full technical architecture | Engineers, contributors | All architectural decisions, coupling system, preset schema | ✅ Current (2026-03-20) |

---

## Level 2 — Living Indexes (Stay In Sync with L1)

| Document | Purpose | Audience | Authority For | Status |
|----------|---------|----------|---------------|--------|
| `seance_cross_reference.md` | Engine quality audit index | Sound designers, agents | Engine health status, seance scores, D-violations | ✅ Current (71 seances as of 2026-03-23) |
| `MANIFEST.md` (this file) | Documentation inventory | All contributors | Which doc to consult for what | ✅ Current |
| `GOVERNANCE.md` | Update policies and SLAs | All contributors | When to update what | ✅ Current |
| `INDEX.md` | Full discovery index | New contributors | Where to find anything | ✅ Current (verify) |
| `../Skills/README.md` | Skill library index | Agents, contributors | Which skill to invoke | ✅ Current (2026-03-20) |

---

## Level 3 — Reference (Updated Per Cycle)

| Document | Purpose | Audience | Authority For | Status |
|----------|---------|----------|---------------|--------|
| `xoutshine-forge-spec.md` | XOutshine universal instrument forge format spec | Export engineers, pipeline devs | SampleCategory enum, multi-source keygroups, velocity strategies, RR spec, formant metadata, FX routing, 9-stage pipeline, XPM examples, MPC constraints | ✅ Current (2026-03-22, v1.0) |
| `xomnibus_sound_design_guides.md` | Per-engine sonic reference | Sound designers | Per-engine parameters, coupling, pairings | ⚠️ 38/71 engines (OBRIX, ORBWEAVE, OVERTONE, ORGANISM, OXBOW, OWARE, OPERA, OFFERING, OSMOSIS, OXYTOCIN + Kitchen Collection 24 pending) |
| `xomnibus_name_migration_reference.md` | Legacy → canonical engine name map | Agents, engineers | Name aliases, gotchas | ✅ Current |
| `xomnibus_landscape_2026.md` | Grand fleet survey | Sound designers, engineers | Pre-sweep baseline metrics | ⚠️ Dated 2026-03-14 (pre-sweep), not updated after Round 12 |
| `fleet_health_2026_03_20.md` | Current fleet status | All | Post-sweep health metrics | ✅ Current (generated 2026-03-20) |
| `documentation_health_plan.md` | Doc improvement roadmap | All contributors | What needs fixing in docs | ✅ Current |
| `../Skills/*/SKILL.md` | Per-skill procedure guides | Agents | Workflow execution | ✅ Most current; see each file's metadata |

---

## Level 4 — Historical (Immutable Archives)

| Document | Purpose | Notes |
|----------|---------|-------|
| `prism_sweep_final_report.md` | 12-round quality sweep final report (2026-03-20) | Complete — do not modify |
| `prism_sweep_index.md` | Round-by-round artifact index | Complete — do not modify |
| `seances/*.md` (71 files) | Individual seance verdict records | Immutable once committed |
| `guild/*.md` | Producer guild + specialist review records | Immutable once committed |
| `../scripture/the-scripture.md` | Empirical wisdom from development | Living but archival in spirit |

**Note:** Original 24 seance verdicts (2026-03-14, ODDFELIX through OWLFISH) exist only in aggregated form in `xomnibus_landscape_2026.md`. Individual verdict files not yet back-generated.

---

## Level 5 — Archive (Add Status Header, Do Not Delete)

| Document | Original Purpose | Why Archived | Canonical Replacement |
|----------|-----------------|-------------|----------------------|
| `xo_mega_tool_preset_system.md` | Original preset system spec | Superseded | `xomnibus_preset_spec_for_builder.md` |
| `xo_mega_tool_visual_identity.md` | Original visual identity spec | Superseded | `xomnibus_technical_design_system.md` |
| `plans/v1-launch-plan.md` | V1 launch roadmap | V1 complete | `prism_sweep_final_report.md` |
| `plans/2026-03-12-xostinato-design.md` | XOSTINATO design | Engine shipped 2026-03-18 | `seances/ostinato_seance_verdict.md` |

---

## Docs Directory Quick Reference

| Directory | Contents | Status |
|-----------|----------|--------|
| `Docs/` root | Core specs, master docs | ✅ Well-maintained |
| `Docs/seances/` | 17 seance verdict files (newest engines) | ✅ Active; original 24 pending |
| `Docs/concepts/` | Engine concept briefs (historical mythology) | ✅ Complete archive |
| `Docs/plans/` | Product roadmaps | ⚠️ Mixed active/completed, needs status headers |
| `Docs/specs/` | 162 R&D + active specs | ⚠️ Disorganized; future sprint to sort active/archive |
| `Docs/guild/` | Producer guild reviews | ✅ Current; consolidation with guild-reports/ pending |
| `Docs/snapshots/` | Timestamped fleet health JSONs | ⚠️ Purpose unclear; pre-sweep artifacts |
| `Docs/ebook/` | XPN format + MPC ebook chapters | ✅ Stable |
| `Docs/packs/` | Pack documentation | ✅ Active |

---

## Skills Quick Reference

| Skill | File | Status |
|-------|------|--------|
| `/coupling-preset-designer` | `Skills/coupling-preset-designer/SKILL.md` | ✅ Active |
| `/coupling-interaction-cookbook` | `Skills/coupling-interaction-cookbook/SKILL.md` | ✅ Active |
| `/mod-matrix-builder` | `Skills/mod-matrix-builder/SKILL.md` | ✅ Active |
| `/preset-architect` | `Skills/preset-architect/SKILL.md` | ✅ Active (updated 2026-03-20: +Submerged mood, +13 engines) |
| `/engine-health-check` | `Skills/engine-health-check/SKILL.md` | ✅ Active |
| `/dna-designer` | `Skills/dna-designer/SKILL.md` | ✅ Active (updated 2026-03-20: 42 engines) |
| `/xpn-export-specialist` | `Skills/xpn-export-specialist/SKILL.md` | ✅ Active |
| `/sro-optimizer` | `Skills/sro-optimizer/SKILL.md` | ✅ Active |
| `/preset-auditor` | `Skills/preset-auditor/SKILL.md` | ✅ Active (new 2026-03-20) |
| `/coupling-debugger` | `Skills/coupling-debugger/SKILL.md` | ✅ Active (new 2026-03-20) |
| `/master-audit` | `Skills/master-audit/SKILL.md` | ✅ Active (new 2026-03-20) |
| `/synth-seance` | `~/.claude/skills/synth-seance/` | ✅ Active |
| `/post-engine-completion-checklist` | `~/.claude/skills/` | ✅ Active |
| `/producers-guild` | `~/.claude/skills/` | ✅ Active |
