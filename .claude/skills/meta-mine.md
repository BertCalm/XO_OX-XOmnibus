# /meta-mine — Cross-Repository Skill Mining

Aggregate skill journals from multiple projects and surface patterns that only become visible at the portfolio level.

## When to Use

- When you have journal entries from 2+ different projects
- When starting a new project type and want to know what primitives to bring
- Quarterly review of your framework's coverage and gaps

## Prerequisites

Set up a central framework location. Either:

**Option A: Dedicated repo** (recommended for version control + portability)
```
~/Projects/skill-framework/       # or any path
  journal/
    aggregated.jsonl              # merged entries from all projects
  primitives/                     # portable across all projects
  skills/                         # cross-project skills
  context/                        # shared context slices
  meta-skills/
    meta-mine.md                  # this file (canonical copy)
```

**Option B: Global config**
```
~/.claude/framework/
  journal/aggregated.jsonl
  primitives/
  skills/
  context/
```

Set `SKILL_FRAMEWORK_HOME` to point to your chosen location.

## Process

### 1. Aggregate Journals

Scan known project paths for `.claude/journal/entries.jsonl` files. Merge all entries into a unified dataset, tagged by source project.

Known project paths to scan (add yours):
```
~/Projects/XO_OX-XOmnibus/.claude/journal/entries.jsonl
# Add more as projects adopt the framework:
# ~/Projects/OddfeliX/.claude/journal/entries.jsonl
# ~/Projects/SomeWebApp/.claude/journal/entries.jsonl
```

If `$SKILL_FRAMEWORK_HOME/journal/aggregated.jsonl` exists, read that instead (it should already be merged by `/log-session`).

### 2. Cross-Project Pattern Analysis

Run the same analysis as `/mine-skills` but with additional cross-project lenses:

**A. Universal Primitives**
Patterns that appear across 2+ *different project types* (not just 2 repos of the same kind):
- `validate-fix-loop` in a synth project AND a web app → truly universal
- `scaffold-then-register` in an engine AND an API → truly universal
- These are your highest-value framework investments

**B. Domain Primitives**
Patterns that repeat within a project type but don't cross domains:
- `generate → compute-dna → validate` only appears in synth projects → domain primitive
- Still valuable, but tag them as domain-specific so they don't clutter other projects

**C. Context Portability Map**
Which context slices from one project have analogs in others?
- XOmnibus has "parameter prefix rules" → Web app has "API naming conventions" → abstract to "naming-rules" context pattern
- XOmnibus has "preset schema" → Web app has "config schema" → abstract to "schema-validation" context pattern

**D. Framework Coverage Score**
For each project, calculate:
- % of logged steps that are covered by an existing primitive or skill
- % of context loads that are covered by a context slice
- Friction points that have no corresponding framework component
- This tells you where the framework is mature vs. where gaps remain

### 3. Generate Cross-Project Report

```markdown
## Meta-Mining Report — {date}

### Portfolio Overview
- Projects tracked: {list with types}
- Total journal entries: {N}
- Framework coverage: {avg %}

### Universal Primitive Candidates
Patterns seen across different project types:
- **`{name}`** — {pattern} — seen in {project_list}
  Portability: Universal | Priority: {score}

### Domain Primitive Candidates
Patterns within a project type:
- **`{name}`** — {pattern} — domain: {project_type}
  Portability: {domain} only | Priority: {score}

### Context Pattern Abstractions
Analogous context slices across projects:
- **`{abstract-name}`**
  - In {project_a}: {concrete context}
  - In {project_b}: {concrete context}
  - Abstract pattern: {what they share}

### Framework Health
| Project | Coverage | Entries | Top Gap |
|---------|----------|---------|---------|
| {name}  | {%}      | {N}     | {gap}   |

### Recommended Framework Additions
1. {Highest priority item with rationale}
2. {Second priority}
3. {Third priority}
```

### 4. Framework Sync

When primitives or skills are approved at the meta level:

1. Write canonical version to `$SKILL_FRAMEWORK_HOME/primitives/` or `skills/`
2. For each project that would benefit, create a symlink or copy:
   ```
   ln -s $SKILL_FRAMEWORK_HOME/primitives/validate-fix-loop.md \
         ~/Projects/{project}/.claude/primitives/validate-fix-loop.md
   ```
3. Or use a manifest approach — each project's `.claude/framework-manifest.json` lists which shared primitives/skills it uses, and a sync script copies them in.

## Framework Evolution Stages

Track where your framework is in maturity:

| Stage | Signal | Action |
|-------|--------|--------|
| **Capture** | < 10 journal entries | Focus on logging, not mining |
| **Identify** | 10-30 entries | Run `/mine-skills` per project, spot local patterns |
| **Extract** | 30-50 entries, 2+ projects | Run `/meta-mine`, extract first universal primitives |
| **Compose** | 5+ primitives extracted | Build skills from primitives, measure coverage |
| **Mature** | 70%+ coverage, 3+ projects | Framework is self-sustaining, mining finds edge cases |

## New Project Bootstrap

When starting a new project, `/meta-mine` can generate a starter kit:

1. Ask: "What type of project is this?" (synth-engine, web-app, cli-tool, library, etc.)
2. Pull all universal primitives
3. Pull domain primitives matching the project type
4. Generate starter `.claude/` directory with relevant primitives, skills, and context templates
5. Pre-populate context templates with project-specific placeholders
