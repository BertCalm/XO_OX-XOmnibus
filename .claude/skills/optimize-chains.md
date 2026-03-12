# /optimize-chains — Skill Chain Discovery & Continuous Improvement

Analyze existing skills and workflows to find chaining opportunities, redundancies, quality gaps, and efficiency improvements.

## When to Use

- After `/mine-skills` surfaces new primitives or skills
- When a workflow feels slow, fragile, or produces inconsistent results
- Periodically (monthly) as a framework health check
- When adding a new skill — check how it connects to existing ones

## What This Skill Does

Three modes of analysis: **Chain**, **Improve**, and **Evolve**.

---

## Mode 1: Chain Discovery

Find skills and primitives that should be connected but aren't.

### Process

1. **Map all skills and primitives** in `.claude/skills/` and `.claude/primitives/`

2. **Build a dependency graph.** For each skill, identify:
   - What it produces (outputs)
   - What it consumes (inputs)
   - What it could feed into next
   - What should have run before it

3. **Find missing links.** Look for:

   **Dangling outputs** — A skill produces something no other skill consumes:
   - `generate_*_presets.py` outputs .xometa files → but nothing auto-triggers validation
   - Signal: chain `generate → validate → compute-dna` as a single flow

   **Repeated preambles** — Multiple skills start with the same setup steps:
   - 3 skills all begin with "load preset schema, scan preset directory"
   - Signal: extract shared preamble as a primitive or context slice

   **Manual bridges** — Places where a human currently connects two automated steps:
   - You run `/validate`, read the report, then manually decide to run `/compute-dna`
   - Signal: automate the decision gate between them

   **Natural sequences** — Skills that are always invoked in the same order:
   - Every time you scaffold, you also register, then validate
   - Signal: compose into a single chained skill

4. **Propose chains.** For each candidate:

```markdown
### Chain: {name}
**Sequence:** skill_a → skill_b → skill_c
**Currently:** Run manually in sequence, {N} times in journal
**Proposed:** Single invocation chains all three with gates between
**Time saved:** ~{estimate} per invocation
**Quality gain:** {what improves — consistency, fewer missed steps, etc.}
```

---

## Mode 2: Continuous Improvement

Look at existing skills and workflows for efficiency, speed, and quality opportunities.

### Efficiency Analysis

For each skill, ask:

**Can steps run in parallel?**
- Sequential steps that don't depend on each other can be parallelized
- Example: DNA computation and naming validation can run simultaneously on preset files

**Are there redundant steps?**
- Same file read/parsed multiple times across a workflow
- Same validation run at multiple stages when once would suffice
- Example: validate_presets.py scans all presets twice if run before and after DNA computation

**Can work be skipped?**
- Incremental processing: only process changed files, not entire library
- Cache results: if DNA was computed and preset hasn't changed, skip
- Early exit: if first validation fails hard, don't continue to later steps

### Speed Analysis

For each skill, ask:

**Where is the bottleneck?**
- File I/O (reading hundreds of preset files)
- Computation (DNA fingerprinting across 1000 presets)
- Human decision (waiting for approval that could be auto-approved with rules)

**Can the bottleneck be reduced?**
- Batch I/O instead of per-file
- Cache intermediate results
- Auto-approve with configurable thresholds ("if validation score > 95%, auto-approve")

### Quality Analysis

For each skill, ask:

**What can go wrong that isn't caught?**
- A preset generates successfully but sounds bad (no audio quality gate)
- A scaffold compiles but has copy-paste artifacts (no content verification)
- An export bundles correctly but won't load in target app (no smoke test)

**Where is quality inconsistent?**
- Sometimes presets get DNA, sometimes they don't (depends on whether you remember)
- Sometimes exports get cover art, sometimes they don't
- Signal: make optional steps mandatory in the chain, or explicitly opt-out

**Where does quality degrade over time?**
- Preset naming conventions drift as library grows
- Context files become stale as code evolves
- Signal: add periodic re-validation or staleness checks

---

## Mode 3: Evolve

Look for structural improvements to the framework itself.

### Skill Composition Patterns

Identify recurring composition shapes:

**Guard → Act → Verify** (most common)
```
check preconditions → do the work → validate the result
```
If most skills follow this shape, make it a first-class primitive.

**Fan-out → Fan-in**
```
split work across targets → collect results → merge report
```
Example: validate all presets per-engine, then merge into one report.

**Cascade**
```
change triggers change triggers change
```
Example: rename engine → update presets → update docs → update tests.
Signal: build a cascade skill that propagates changes automatically.

### Context Evolution

**Stale context detection:**
- Compare context file timestamps against source files they describe
- If `context/preset-rules.md` references schema version 1 but schema is now version 2, flag it
- If a context file hasn't been updated in 3+ months but its source files have changed, flag it

**Context splitting:**
- If a context file is loaded frequently but only 20% of it is used, split it
- Track which sections of context are actually referenced (via journal `context_loaded` entries)

**Context merging:**
- If two context files are always loaded together, merge them
- Reduces load overhead and keeps related knowledge co-located

### Primitive Evolution

**Promotion:** When a project-specific step appears in 3+ skills, extract it as a primitive.

**Generalization:** When two primitives are similar but not identical, find the common abstraction:
- `validate-presets` and `validate-recipes` → generalize to `validate-schema(target, schema)`

**Deprecation:** When a primitive is no longer used by any skill, flag it for removal.

---

## Output Format

```markdown
## Chain & Optimization Report — {date}

### New Chain Candidates
For each:
- **{Chain name}**: {skill_a} → {skill_b} → {skill_c}
- **Why:** {what triggers this sequence, how often}
- **Gain:** {time, quality, consistency}
- [ ] Approve → create as `.claude/skills/{chain-name}.md`

### Efficiency Improvements
For each:
- **{Skill name}**: {specific improvement}
- **Type:** Parallel | Skip | Cache | Batch
- **Gain:** {estimate}
- [ ] Approve → update skill definition

### Quality Gaps
For each:
- **{Gap}**: {what can go wrong, in which workflow}
- **Fix:** {add gate | add check | make step mandatory}
- **Risk if unfixed:** {consequence}
- [ ] Approve → add to relevant skill

### Framework Health
- Skills: {N} total, {N} chainable, {N} standalone
- Primitives: {N} total, {N} in use, {N} unused
- Context: {N} total, {N} stale, {N} oversized
- Coverage: {%} of journal steps covered by framework
- Staleness: {list of context files needing refresh}
```

## Feeding Back Into the Framework

Approved improvements should:
1. Update the relevant skill/primitive/context files
2. Log the improvement to the journal as `task_type: "framework-improvement"`
3. Increment `framework_version` in `framework-manifest.json`
4. If cross-repo applicable, sync to `$SKILL_FRAMEWORK_HOME`

This creates a virtuous cycle:
```
Work → Log → Mine → Discover chains → Optimize → Better work → Log → ...
```
