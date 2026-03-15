# /discover-skills — Adjacent Skill Discovery

Surface skills, primitives, tools, and techniques you might not know you need — based on what you're currently doing and what experienced developers typically pair with that work.

## When to Use

- At the **start** of a task, to see what related capabilities might help
- **During** work, when you feel stuck or sense there's a better way
- When you're a **novice** in a domain and want to discover unknown unknowns
- Proactively triggered by other skills when they detect you might benefit

## Process

### 1. Understand the Current Task

Look at what the user is doing right now:
- What files are being edited?
- What type of work is it? (building, debugging, configuring, designing, deploying)
- What tools have been used so far this session?
- What domain is this? (audio DSP, web frontend, API backend, DevOps, data, etc.)

### 2. Map to Skill Adjacencies

For each task type, there are skills that experienced developers almost always pair with it. Surface these as recommendations.

#### Adjacency Map

**When building new code:**
- Testing (unit, integration, snapshot) — "You wrote new code. How will you know it works?"
- Linting/formatting — "Consistent style prevents bugs you won't see for weeks"
- Type checking — "Catch errors before runtime"
- Documentation — "Future-you will thank present-you"
- Error handling patterns — "What happens when this fails?"

**When fixing a bug:**
- Git bisect — "Find exactly which commit introduced it"
- Logging/debugging tools — "See what's actually happening vs. what you think"
- Regression testing — "Ensure this specific bug can't come back"
- Root cause analysis — "Is this a symptom of a deeper issue?"

**When doing DSP/audio work:**
- Denormal protection — "Tiny numbers that tank your CPU"
- Benchmarking — "Does this run within budget at 44.1kHz?"
- A/B listening tests — "Does this actually sound different/better?"
- Parameter smoothing — "Avoid zipper noise on control changes"
- Buffer safety — "Are you reading/writing within bounds?"

**When working with presets/configs:**
- Schema validation — "Does this file actually match the expected format?"
- Migration tooling — "How do you handle format changes over time?"
- Diff tooling — "Can you see what actually changed between versions?"
- Default values — "What happens when a field is missing?"

**When deploying/exporting:**
- Smoke tests — "Does the output actually load in the target environment?"
- Size budgets — "Is the output a reasonable size?"
- Dependency checking — "Are all required files included?"
- Rollback plan — "How do you undo this if it's wrong?"

**When refactoring:**
- Test coverage check — "Do tests cover the code you're about to change?"
- Dead code detection — "What can you safely remove?"
- Dependency graph — "What else depends on what you're changing?"
- Incremental verification — "Can you refactor in small, testable steps?"

**When starting a new project:**
- Project scaffold — "Standard directory structure for this type of project"
- CI/CD setup — "Automate build/test from day one"
- Dependency management — "Lock versions, audit for vulnerabilities"
- Git conventions — "Branch strategy, commit message format, PR template"

### 3. Check What's Already Available

Before recommending, check what already exists:
- Scan `.claude/skills/` for skills that address the adjacency
- Scan `.claude/primitives/` for relevant primitives
- Scan `Tools/` for existing scripts that cover it
- Check if `$SKILL_FRAMEWORK_HOME` has cross-project skills that apply

### 4. Score Relevance

For each adjacent skill, assess:

| Factor | Weight | Description |
|--------|--------|-------------|
| Relevance | 3x | How directly related to current work? |
| Risk | 2x | How bad if you skip this? (high = recommend strongly) |
| Availability | 1x | Is there already a tool/skill for this? (exists = just remind, doesn't exist = suggest creating) |
| Skill level | 2x | Is this something a novice would likely miss? (yes = recommend strongly) |

### 5. Present Recommendations

Output a brief, scannable list grouped by urgency:

```markdown
## Skill Discovery — {task description}

### You should probably do this now
- **{Skill}** — {why, in one sentence}
  Status: {Available as /skill-name | Available as Tools/script.py | Not yet built}

### Worth knowing about
- **{Skill}** — {why, in one sentence}
  Status: {same}

### For later / deeper dive
- **{Skill}** — {why, in one sentence}
  Status: {same}
```

Keep it to 3-5 recommendations max. Don't overwhelm — prioritize the ones most likely to prevent mistakes or save significant time.

### 6. Learning Hooks

For novice users, add a one-line explanation of *why* this matters, not just *what* it is:

- Bad: "Consider adding unit tests"
- Good: "Unit tests catch bugs before they reach your users — and they make refactoring safe because you'll know immediately if you broke something"

- Bad: "Use schema validation"
- Good: "Schema validation means a typo in a preset file gets caught in 1 second instead of surfacing as a mysterious bug 3 weeks from now"

### 7. Log to Journal

If the user acts on a recommendation, note it in the session journal under `steps` with:
```json
{"action": "discovered", "target": "{skill name}", "tool": "discover-skills", "repeatable": false}
```

This feeds back into `/mine-skills` — if users frequently act on the same type of recommendation, that's a signal to make it a default part of the workflow rather than a suggestion.

## Proactive Trigger Points

Other skills can invoke discovery suggestions at natural moments:

| Trigger | Suggestion |
|---------|------------|
| After `/new-engine` scaffold | "Consider: testing setup, parameter range validation, benchmark" |
| After preset generation | "Consider: validation, DNA computation, listening test" |
| After any export | "Consider: smoke test on target platform, size check" |
| After a bug fix | "Consider: regression test, root cause check" |
| After large refactor | "Consider: test run, dead code scan, dependency check" |
