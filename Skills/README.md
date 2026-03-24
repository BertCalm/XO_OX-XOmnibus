# XOlokun Skill Library

This directory contains Claude Code skill files for working efficiently with the XOlokun codebase. Each skill is a focused procedure guide that teaches Claude how to perform a specific workflow with maximum quality and minimum back-and-forth.

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
| [skill-friction-detective](./skill-friction-detective/SKILL.md) | `/skill-friction-detective` | Meta-skill: detect friction, frustration, silent failures, and orphaned threads across all skills — then repair, link, and optimize | HIGH |
| [midi-daw-audit](./midi-daw-audit/SKILL.md) | `/midi-daw-audit` | MIDI/DAW compatibility audit — CC thread safety, MIDI learn wiring, APVTS exposure, state persistence, MPE, pitch bend, sustain, transport sync, plugin contract | HIGH |
| [sro-optimizer](./sro-optimizer/SKILL.md) | `/sro-optimizer` | Spectral Resonance Object CPU optimization — audit, integrate SilenceGate, fleet-wide sweep | MEDIUM |
| [preset-auditor](./preset-auditor/SKILL.md) | `/preset-auditor` | Quality gate for existing presets — DNA coverage, macro responsiveness, D004 compliance | HIGH |
| [coupling-debugger](./coupling-debugger/SKILL.md) | `/coupling-debugger` | Diagnose and fix broken or inaudible coupling routes | HIGH |
| [master-audit](./master-audit/SKILL.md) | `/master-audit` | Orchestrate all QA checks into a fleet-wide health report | HIGH |
| [repo-audit](./repo-audit/SKILL.md) | `/repo-audit` | Repo hygiene and documentation currency — stale counts, broken IDs, ODR hazards, governance gaps | HIGH |
| [preset-audit-checklist](./preset-audit-checklist/SKILL.md) | `/preset-audit-checklist` | 7-phase Guru-informed preset audit — 10-point sonic gate, engine-specific depth, coverage gaps, Guru tricks. Path to 9.0+ preset libraries. | HIGH |
| [new-xo-engine](./new-xo-engine/SKILL.md) | `/new-xo-engine` | End-to-end new engine creation — concept brief, scaffold, integration, verification | HIGH |
| [session-start-hook](./session-start-hook/SKILL.md) | `/session-start-hook` | Configure Claude Code SessionStart hook for web sessions — tests, linters, project context | LOW |

### Referenced Elsewhere (`~/.claude/skills/`)

These skills exist in the Claude profile (not version-controlled here) but are referenced throughout the Docs:

| Skill | Invoke | Purpose | Source |
|-------|--------|---------|--------|
| synth-seance | `/synth-seance` | Ghost council evaluation of engine quality against 6 doctrines and 15 blessings | `Docs/seance_cross_reference.md` |
| post-engine-completion-checklist | `/post-engine-completion-checklist` | 5-point audit after engine completion: CLAUDE.md accuracy, XOlokun sync, knowledge compendium, memory/satellite, playbook | `Docs/knowledge_tree_update_12i.md` |
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
| Optimizing engine CPU / constraint-driven DSP | `/sro-optimizer` |
| Pre-release MIDI/DAW compatibility check | `/midi-daw-audit` |
| CC not working in a DAW / automation lost after reopen | `/midi-daw-audit` |
| Deep engine quality evaluation | `/synth-seance` |
| After finishing an engine | `/post-engine-completion-checklist` |
| Product direction / market review | `/producers-guild` |
| Building a new engine from scratch | `/new-xo-engine` |
| A skill produced confusion, loops, or silent failures | `/skill-friction-detective` |
| Auditing the skill library for stale content or orphaned skills | `/skill-friction-detective audit` |
| Finding cross-skill linking opportunities | `/skill-friction-detective link` |
| Auditing existing preset quality (quick) | `/preset-auditor` |
| Full preset audit targeting 9.0+ quality | `/preset-audit-checklist` |
| Guru-informed preset refinement | `/preset-audit-checklist` + `/guru-bin` |
| Coupling route not working/inaudible | `/coupling-debugger` |
| Full fleet QA / pre-release health check | `/master-audit` |
| Stale counts, broken IDs, governance gaps | `/repo-audit` |
| CPU optimization for an engine | `/sro-optimizer` |

---

## Related Documentation

- `Docs/coupling_audit.md` — Coupling quality scores for all engines
- `Docs/coupling_preset_library.md` — 18 proven coupling preset examples with mythology
- `Docs/xolokun_sound_design_guides.md` — Per-engine parameter reference (38 of 42 engines; OBRIX/ORBWEAVE/OVERTONE/ORGANISM pending)
- `Docs/sonic_dna_audit.md` — DNA coverage analysis + gap table
- `Docs/how_to_write_a_xolokun_adapter.md` — Adapter writing guide
- `Docs/xolokun_new_engine_process.md` — New engine development process
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
