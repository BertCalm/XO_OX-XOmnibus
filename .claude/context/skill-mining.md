# Context: Skill Mining Framework

## What This Framework Is

A three-layer system for externalizing development expertise into callable, reusable components:

- **Primitives** — Generic, portable building blocks (verbs with no opinion)
- **Skills** — Opinionated workflows composed from primitives (project-specific recipes)
- **Context** — Just-in-time knowledge that makes skills produce correct output

## Framework Directory Structure

```
.claude/
  skills/           # Callable workflows (invoked as /skill-name)
  primitives/       # Portable building blocks (referenced by skills)
  context/          # Knowledge slices (loaded by skills as needed)
  journal/
    entries.jsonl   # Append-only session log
    schema.json     # Entry format specification
  framework-manifest.json  # Which shared components this project uses
```

## Available Skills

| Skill | Purpose | Trigger |
|-------|---------|---------|
| `/log-session` | Record session work to journal | End of substantive session |
| `/mine-skills` | Analyze journal for framework candidates | Periodic (weekly or 10+ entries) |
| `/meta-mine` | Cross-repo pattern analysis | 2+ projects with journals |
| `/discover-skills` | Surface adjacent skills you might need | During or before a task |
| `/optimize-chains` | Find chain opportunities and improvements | After mining or monthly review |

## Available Primitives

| Primitive | Pattern | Portable? |
|-----------|---------|-----------|
| `validate-fix-loop` | Run validator → fix → re-validate | Universal |
| `scaffold-and-register` | Template → create → register → verify | Universal |
| `bulk-transform` | Find files → transform → report | Universal |
| `chain-pipeline` | Step → gate → step → gate → done | Universal |
| `cross-repo-sync` | Sync framework components across repos | Universal |

## Journal Entry Fields

Key fields captured per session:
- `task_type` — What kind of work (scaffold, validate, generate, migrate, etc.)
- `steps` — Ordered actions with tools and repeatability flags
- `context_gaps` — Info you had to hunt for (signals missing context)
- `friction_points` — What slowed you down (signals automation opportunity)
- `pattern_tags` — Freeform clustering tags for mining

## Cross-Repo Setup

Set `SKILL_FRAMEWORK_HOME` to a central location:
```bash
export SKILL_FRAMEWORK_HOME=~/Projects/skill-framework
```

The `/log-session` skill will sync entries to the central journal.
The `/meta-mine` skill reads the central journal for cross-project patterns.
