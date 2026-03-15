# XOmnibus Skill Library

This directory contains Claude Code skill files for working efficiently with the XOmnibus codebase. Each skill is a focused procedure guide that teaches Claude how to perform a specific workflow with maximum quality and minimum back-and-forth.

---

## Available Skills

### In This Repository (`Skills/`)

| Skill | Invoke | Purpose | Priority |
|-------|--------|---------|----------|
| [coupling-preset-designer](./coupling-preset-designer/SKILL.md) | `/coupling-preset-designer` | Design coupling presets from scratch — engine pair selection, CouplingType mapping, DNA, quality gate | HIGH |
| [coupling-interaction-cookbook](./coupling-interaction-cookbook/SKILL.md) | `/coupling-interaction-cookbook` | Quick-reference for best engine pairings, proven routes, coupling type index | HIGH |
| [mod-matrix-builder](./mod-matrix-builder/SKILL.md) | `/mod-matrix-builder` | Reusable 8-slot mod matrix template covering D002/D005/D006 requirements | MEDIUM |
| [preset-architect](./preset-architect/SKILL.md) | `/preset-architect` | Create high-quality `.xometa` presets — parameters, macros, DNA, naming, quality gate | MEDIUM |
| [engine-health-check](./engine-health-check/SKILL.md) | `/engine-health-check` | Quick D001–D006 doctrine compliance check + coupling quality + known bugs | MEDIUM |
| [dna-designer](./dna-designer/SKILL.md) | `/dna-designer` | Assign accurate 6D Sonic DNA values — calibrated per dimension per engine | MEDIUM |
| [xpn-export-specialist](./xpn-export-specialist/SKILL.md) | `/xpn-export-specialist` | End-to-end XPN/MPC export — render specs, WAV rendering, XPM rules, bundle assembly | MEDIUM |

### Referenced Elsewhere (`~/.claude/skills/`)

These skills exist in the Claude profile (not version-controlled here) but are referenced throughout the Docs:

| Skill | Invoke | Purpose | Source |
|-------|--------|---------|--------|
| synth-seance | `/synth-seance` | Ghost council evaluation of engine quality against 6 doctrines and 15 blessings | `Docs/seance_cross_reference.md` |
| post-engine-completion-checklist | `/post-engine-completion-checklist` | 5-point audit after engine completion: CLAUDE.md accuracy, XOmnibus sync, knowledge compendium, memory/satellite, playbook | `Docs/knowledge_tree_update_12i.md` |
| producers-guild | `/producers-guild` | 12 genre specialist + PM + market research product review | `Docs/2026-03-14-session-readout.md` |

---

## Skill Selection Guide

| Situation | Use This Skill |
|-----------|---------------|
| Writing a new Entangled mood preset | `/coupling-preset-designer` |
| Need to know which engines pair well | `/coupling-interaction-cookbook` |
| Engine failing D002/D005/D006 doctrine | `/mod-matrix-builder` |
| Writing any new `.xometa` preset | `/preset-architect` |
| Quick engine QA before release | `/engine-health-check` |
| Adding DNA to existing presets | `/dna-designer` |
| Building an MPC export bundle | `/xpn-export-specialist` |
| Deep engine quality evaluation | `/synth-seance` |
| After finishing an engine | `/post-engine-completion-checklist` |
| Product direction / market review | `/producers-guild` |
| Building a new engine from scratch | `/new-xo-engine` (see `Docs/xomnibus_new_engine_process.md`) |

---

## Related Documentation

- `Docs/coupling_audit.md` — Coupling quality scores for all 31 engines
- `Docs/coupling_preset_library.md` — 18 proven coupling preset examples with mythology
- `Docs/xomnibus_sound_design_guides.md` — Per-engine parameter reference (20 of 31 engines)
- `Docs/sonic_dna_audit.md` — DNA coverage analysis + gap table
- `Docs/how_to_write_a_xomnibus_adapter.md` — Adapter writing guide
- `Docs/xomnibus_new_engine_process.md` — New engine development process
- `Docs/seance_cross_reference.md` — All engine seance scores + D-violations + P0 bugs

---

## Skill Format

Each skill follows this structure:

```
Skills/{skill-name}/
└── SKILL.md          # The complete skill guide
```

SKILL.md contains:
1. **When to Use** — exact situations that trigger this skill
2. **Phases/Steps** — ordered procedure
3. **Templates** — copy-paste code, JSON, or command patterns
4. **Quality Gates** — checklists to verify work before finishing
5. **Related Resources** — links to docs, tools, reference engines
