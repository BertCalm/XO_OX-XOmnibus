# XOceanus Documentation Manifest
**Last Updated:** 2026-04-26
**Purpose:** Canonical inventory of all major documentation — purpose, audience, authority level, and currency status.

For governance rules (when to update what), see `GOVERNANCE.md`.
For the detailed documentation improvement plan, see `fleet-audit/documentation_health_plan.md`.

---

## Level 1 — Canonical (Must Stay Current)

| Document | Purpose | Audience | Authority For | Status |
|----------|---------|----------|---------------|--------|
| `../CLAUDE.md` | Master project guide for Claude agents and contributors | Agents, all contributors | Engine registry, blessings, debates, doctrines, architecture rules | ✅ Current (2026-04-03) |
| `xoceanus_master_specification.md` | Full technical architecture | Engineers, contributors | All architectural decisions, coupling system, preset schema | ✅ Current (2026-03-20) |

---

## Level 2 — Living Indexes (Stay In Sync with L1)

| Document | Purpose | Audience | Authority For | Status |
|----------|---------|----------|---------------|--------|
| `seances/seance_cross_reference.md` | Engine quality audit index | Sound designers, agents | Engine health status, seance scores, D-violations | ✅ Current (74 seances as of 2026-04-03; OBIONT, OUTFLOW not yet seanced) |
| `MANIFEST.md` (this file) | Documentation inventory | All contributors | Which doc to consult for what | ✅ Current |
| `GOVERNANCE.md` | Update policies and SLAs | All contributors | When to update what | ✅ Current |
| `INDEX.md` | Full discovery index | New contributors | Where to find anything | ✅ Current (verify) |
| `../Skills/README.md` | Skill library index | Agents, contributors | Which skill to invoke | ✅ Current (2026-03-20) |

---

## Level 3 — Reference (Updated Per Cycle)

| Document | Purpose | Audience | Authority For | Status |
|----------|---------|----------|---------------|--------|
| `design/xoutshine-forge-spec.md` | XOutshine universal instrument forge format spec | Export engineers, pipeline devs | SampleCategory enum, multi-source keygroups, velocity strategies, RR spec, formant metadata, FX routing, 9-stage pipeline, XPM examples, MPC constraints | ✅ Current (2026-03-22, v1.0) |
| `xoceanus_sound_design_guides.md` | Per-engine sonic reference | Sound designers | Per-engine parameters, coupling, pairings | ⚠️ 71/77 engines (5 pending: OCELOT, OSPREY, OBIONT, OKEANOS, OUTFLOW) |
| `xoceanus_name_migration_reference.md` | Legacy → canonical engine name map | Agents, engineers | Name aliases, gotchas | ✅ Current |
| `plans/xoceanus_landscape_2026.md` | Grand fleet survey | Sound designers, engineers | Pre-sweep baseline metrics | ⚠️ Dated 2026-03-14 (pre-sweep), not updated after Round 12 |
| `fleet-audit/fleet_health_2026_03_20.md` | Current fleet status | All | Post-sweep health metrics | ✅ Current (generated 2026-03-20) |
| `fleet-audit/documentation_health_plan.md` | Doc improvement roadmap | All contributors | What needs fixing in docs | ✅ Current |
| `fleet-audit/fathom-qdd-level5-fleet-certification-2026-03-29.md` | FATHOM × QDD Level 5 pre-launch fleet certification | Engineers, sound designers, agents | 77-engine audit results, 20-agent findings, 47 CRITs, ship-ready list, launch blockers | ✅ Current (2026-03-29) |
| `fleet-audit/fathom-fleet-report-2026-03-29.md` | FATHOM brutal sonic audit — all 77 engines + DSP library | Sound designers, agents | Per-engine sonic quality verdicts, FATHOM scores, systemic issues | ✅ Current (2026-03-29) |
| `design/tidesigns-audit.md` | TIDEsigns QDD full-fleet UI/UX audit | UI engineers, agents | 450+ findings, 8-wave fix sequence, tier priorities, score history | ✅ Current (2026-03-29) |
| `fleet-audit/xoceanus-fleet-inventory-2026-03-28.md` | Complete engine fleet inventory — all 77 engines | All contributors, agents | Engine roster, build status, preset counts, seance scores | ✅ Current (2026-03-28) |
| `specs/export-architecture.md` | Export Pyramid architecture — ORIGINATE → OUTSHINE → OXPORT | Export engineers, pipeline devs | Three-tool export chain design, tool responsibilities, data flow | ✅ Current (2026-03-29) |
| `../Skills/*/SKILL.md` | Per-skill procedure guides | Agents | Workflow execution | ✅ Most current; see each file's metadata |

---

## Level 4 — Historical (Immutable Archives)

| Document | Purpose | Notes |
|----------|---------|-------|
| `prism_sweep_final_report.md` | 12-round quality sweep final report (2026-03-20) | Complete — do not modify |
| `prism_sweep_index.md` | Round-by-round artifact index | Complete — do not modify |
| `seances/*.md` (42 files across `Docs/seances/` + `scripture/seances/`) | Individual seance verdict records | Immutable once committed |
| `guild/*.md` | Producer guild + specialist review records | Immutable once committed |
| `../scripture/the-scripture.md` | Empirical wisdom from development | Living but archival in spirit |

**Note:** Original 24 seance verdicts (2026-03-14, ODDFELIX through OWLFISH) exist only in aggregated form in `xoceanus_landscape_2026.md`. Individual verdict files not yet back-generated.

---

## Level 5 — Archive (Add Status Header, Do Not Delete)

| Document | Original Purpose | Why Archived | Canonical Replacement |
|----------|-----------------|-------------|----------------------|
| `xo_mega_tool_preset_system.md` | Original preset system spec | Superseded | `xoceanus_preset_spec_for_builder.md` |
| `xo_mega_tool_visual_identity.md` | Original visual identity spec | Superseded | `xoceanus_technical_design_system.md` |
| `plans/v1-launch-plan.md` | Launch roadmap (RETIRED 2026-04-16 — V1-gating framing abandoned) | Historical | `prism_sweep_final_report.md` |
| `plans/2026-03-12-xostinato-design.md` | XOSTINATO design | Engine shipped 2026-03-18 | `seances/ostinato_seance_verdict.md` |

---

## Docs Directory Quick Reference

| Directory | Contents | Status |
|-----------|----------|--------|
| `Docs/` root | Core specs, master docs | ✅ Well-maintained |
| `Docs/design/` | 15 UI/design spec files incl. spatial architecture V1, pixel-art creature spec, definitive UI spec, playsurface design, accessibility, button/toast/input-state systems, asset registry, Figma compendium | ✅ Active (2026-03-22 to 2026-03-24) |
| `Docs/seances/` | 24 seance verdict files (newest engines) | ✅ Active; original 24 (pre-sweep) exist only in aggregated form in `xoceanus_landscape_2026.md` |
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
