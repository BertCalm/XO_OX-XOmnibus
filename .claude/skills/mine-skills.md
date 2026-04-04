# /mine-skills — Discover Framework Candidates

Analyze the skill journal to surface repeating patterns worth promoting to reusable primitives, skills, or context slices.

## When to Use

- Periodically (weekly, or after 10+ journal entries accumulate)
- When starting a new project and wondering what to bring from previous work
- When a session felt repetitive and you want to confirm the pattern

## Process

### 1. Load the Journal

Read `.claude/journal/entries.jsonl`. If fewer than 3 entries exist, note that more data is needed and suggest running `/log-session` after future work.

### 2. Mine for Patterns

Analyze entries across these dimensions:

**A. Repeated Step Sequences (Primitive Candidates)**
- Find step sequences (2+ actions) that appear across 2+ entries
- Rank by frequency x number of distinct projects
- These become **primitive** candidates — generic, portable building blocks
- Example: `validate → fix → re-validate` appears 8 times → candidate: `validate-fix-loop` primitive

**B. Repeated Task Workflows (Skill Candidates)**
- Find task_types that share similar step lists across entries
- Look for tasks that always load the same context
- These become **skill** candidates — opinionated workflows composed from primitives
- Example: Every `scaffolding` task does `scaffold → register → validate → document` → candidate: `/scaffold-and-register` skill

**C. Repeated Context Loading (Context Slice Candidates)**
- Find context files loaded together frequently
- Find context_gaps that recur (same info hunted for repeatedly)
- These become **context** candidates — pre-packaged knowledge slices
- Example: `xometa_schema.json` + `preset_spec_for_builder.md` always loaded together → bundle as `preset-rules` context

**D. Friction Point Clusters**
- Group friction_points by similarity
- Recurring friction = highest-priority automation target
- Example: "Had to re-explain parameter prefix rules" x3 → missing context slice

**E. Cross-Project Echoes (Meta-Framework Candidates)**
- If cross-repo data is available (`$SKILL_FRAMEWORK_HOME/journal/aggregated.jsonl`), find patterns that span projects
- These are the most valuable — they're truly portable
- Example: `scaffold-then-register` appears in XOceanus, OddfeliX, and a web app → universal primitive

### 3. Score and Rank Candidates

For each candidate, assess:

| Dimension | Score 1-3 | Description |
|-----------|-----------|-------------|
| Frequency | How often does this pattern appear? |
| Portability | Would this work in other projects? |
| Effort | How hard to formalize? (inverse: 3 = easy) |
| Time Saved | How much manual work does it eliminate? |

**Priority = Frequency x Portability x Effort x Time Saved**

### 4. Generate the Report

Output a structured report:

```markdown
## Skill Mining Report — {date}

### Journal Stats
- Entries analyzed: {N}
- Projects covered: {list}
- Date range: {earliest} to {latest}

### Primitive Candidates (portable building blocks)
For each candidate:
- **Name:** `{suggested-name}`
- **Pattern:** {step sequence}
- **Seen:** {count}x across {projects}
- **Score:** {priority score}
- **Implementation:** {one-line description of what to build}

### Skill Candidates (project workflows)
For each candidate:
- **Name:** `/{suggested-command}`
- **Composes:** {which primitives}
- **Seen:** {count}x
- **Score:** {priority score}
- **Implementation:** {one-line description}

### Context Slice Candidates
For each candidate:
- **Name:** `{suggested-name}`
- **Bundles:** {which files/knowledge}
- **Gaps filled:** {what friction it removes}

### Friction Hotspots
- {Recurring friction point} — seen {N}x — suggested fix: {recommendation}

### Actions
For each candidate above:
- [ ] Approve → scaffold into .claude/{primitives|skills|context}/
- [ ] Defer → revisit next mining pass
- [ ] Reject → not worth formalizing
```

### 5. Act on Approvals

For approved candidates:
- **Primitives:** Create `.claude/primitives/{name}.md` with the generic pattern
- **Skills:** Create `.claude/skills/{name}.md` with the composed workflow
- **Context:** Create `.claude/context/{name}.md` with the bundled knowledge
- If cross-repo, also write to `$SKILL_FRAMEWORK_HOME/{primitives|skills|context}/`

## Mining Heuristics

**Strong signals for promotion:**
- Same 3+ step sequence in 3+ entries
- Same context_gap in 2+ entries
- Same friction_point in 2+ entries
- Pattern appears across 2+ different projects
- Step marked `repeatable: true` that isn't yet automated

**Weak signals (defer, don't promote yet):**
- Pattern seen only once
- Pattern only in one project type
- High effort to formalize relative to time saved
- Pattern is evolving (not yet stable)
