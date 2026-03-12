# /composite — Named Multi-Skill Chains

Define and run named workflows that chain multiple skills in sequence. Turns common multi-step processes into single invocations.

## Usage

- `/composite {name}` — Run a named composite workflow
- `/composite list` — Show all defined composites
- `/composite define {name} {skill1} {skill2} ...` — Define a new composite
- `/composite describe {name}` — Show what a composite does before running it

## Built-in Composites

### /full-audit
**Chain:** `/lint` → `/test` → `/benchmark` → `/debug-audio` → `/preset-qa` → `/feature-review --all` → `/recommend`

Full project health check. Runs every validation skill, then synthesizes findings into recommendations.

**When to use:** Before a release, after a large change, or when you want a complete picture of project health.

**Expected output:** A comprehensive report with maturity scores, performance data, preset health, and a prioritized action list.

### /engine-check {engine}
**Chain:** `/research {engine}` → `/feature-review {engine}` → `/debug-audio --engine {engine}` → `/recommend {engine}`

Deep evaluation of a single engine's implementation, health, and improvement opportunities.

**When to use:** After modifying an engine, before shipping an engine, or when an engine seems off.

### /ship-ready
**Chain:** `/migrate --audit` → `/lint` → `/test` → `/benchmark` → `/preset-qa` → `/release --check`

Quick release-readiness check. Lighter than `/full-audit` — focuses on "can we ship?" not "what should we improve?"

**When to use:** When you think you're ready to release and want final confirmation.

### /new-sound {mood}
**Chain:** `/coupling-design wild` → `/preset-breed mood {mood}` → `/preset-qa`

Creative workflow: generate experimental coupling ideas, breed a preset around them, validate the result.

**When to use:** When you want to explore new sonic territory within a mood category.

### /deep-dive {topic}
**Chain:** `/research {topic}` → `/feature-review {topic}` → `/recommend {topic}`

The full strategy chain — investigate, evaluate, and recommend.

**When to use:** When you want to understand a feature area thoroughly before deciding what to do.

## Process

### Running a composite

1. **Announce** the chain: show which skills will run in what order
2. **Execute** each skill sequentially, passing relevant context forward
3. **Gate** on failures: if a validation skill finds critical issues, pause and ask whether to continue or fix first
4. **Summarize** at the end: combine all skill outputs into a single summary

### Context passing

Each skill in the chain can access findings from previous skills:
- `/research` findings are available to `/feature-review`
- `/feature-review` scores are available to `/recommend`
- `/lint` violations are available to `/refactor`
- All validation results are available to `/release`

### Failure handling

| Failure Type | Behavior |
|-------------|----------|
| Validation finds critical issues | Pause, report, ask user whether to fix or continue |
| Skill encounters an error | Report, skip that skill, continue chain, note in summary |
| User interrupts | Stop chain, report progress so far |

### Defining custom composites

Users can define project-specific chains:

```markdown
## Custom Composite: {name}
- **Chain:** /skill1 → /skill2 → /skill3
- **Description:** {what this workflow accomplishes}
- **Gate conditions:** {when to pause — e.g., "if /test fails, stop"}
```

Store custom composites as comments in this file or as separate files in `.claude/composites/`.

## Output Format

```markdown
## Composite: {name}
- **Skills:** {list}
- **Duration:** {approximate time}

### Step 1: /{skill1}
{Summary of findings — not the full output, just key results}
**Status:** ✓ Pass / ⚠ Warnings / ✗ Failed

### Step 2: /{skill2}
{Summary}
**Status:** ✓ / ⚠ / ✗

...

### Summary
- **Overall status:** {Green / Yellow / Red}
- **Key findings:** {3-5 most important takeaways across all skills}
- **Action items:** {if /recommend was in the chain, its top 3 recommendations}
```

## Notes
- Composites are convenience, not magic — each skill runs the same as it would standalone
- The value is in context passing and combined reporting
- Don't create composites for 2-skill chains — just run them manually
- Custom composites should be committed to the repo so all team members can use them
- Gate conditions prevent wasting time running downstream skills when upstream ones fail critically
