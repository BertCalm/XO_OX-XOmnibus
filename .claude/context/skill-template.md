# Skill Template

Standard structure for all XOmnibus framework skills. Use this template when creating new skills to ensure consistency across the framework.

## Template

```markdown
# /{skill-name} — {One-Line Description}

{2-3 sentence overview of what this skill does, when to use it, and what value it provides.}

## Usage

- `/{skill-name}` — {default invocation}
- `/{skill-name} {arg}` — {with argument}
- `/{skill-name} --flag` — {with flag}

## Process

### 1. {First step name}

{Description of what happens in this step.}
{Include any analysis, checks, or preparation.}

### 2. {Second step name}

{Description. Include decision points, branching logic.}

### 3. {Nth step name}

{Description. Include output format.}

## Output Format

{Show the structured output this skill produces.}
{Use markdown tables and code blocks.}

```markdown
## {Skill Name}: {subject}
- **Key metric:** {value}

### {Section}
| Column | Column | Column |
|--------|--------|--------|
| data | data | data |

### Summary
{synthesis}
```

## Primitives Used
- **{primitive-name}** — {how this skill uses the primitive}
- Or: "None — this is a {read-only / analysis / design} skill"

## Relationship to Other Skills
- Fed by: {which skills produce input for this one}
- Feeds into: {which skills consume this one's output}
- Validates with: {which skills verify this one's output}
- Related: {skills that cover adjacent concerns}

## Notes
- {Important constraints or rules}
- {XOmnibus-specific considerations}
- {Common pitfalls to avoid}
```

## Conventions

### Naming
- Skill name: lowercase, hyphenated, 1-2 words (e.g., `debug-audio`, `preset-breed`)
- File: `.claude/skills/{skill-name}.md`
- Title: `/{skill-name} — {Title Case Description}`

### Categories
Every skill belongs to one category:

| Category | Purpose | Examples |
|----------|---------|---------|
| **Meta** | Framework management and self-improvement | log-session, mine-skills, discover-skills |
| **Engineering** | Build, test, validate, debug | build, test, lint, benchmark, debug-audio |
| **Strategy** | Research, evaluate, recommend | research, feature-review, recommend |
| **Workflow** | Lifecycle and process management | migrate, release, refactor, composite |
| **Creative** | Sound design and preset creation | preset-breed, coupling-design |

### Read-only vs. Mutating
Clearly indicate whether a skill modifies files:
- **Read-only skills:** research, feature-review, recommend, recall, coupling-design (analysis mode)
- **Mutating skills:** build, refactor, migrate, preset-breed, release

### Process Steps
- Number steps sequentially
- Each step should be independently understandable
- Include decision points ("if X, then Y; otherwise Z")
- End with output format

### Output Format
- Always include a structured output template
- Use markdown tables for tabular data
- Use code blocks for configs/commands
- Include a summary section

### Primitives
- Reference primitives from `.claude/primitives/` when the skill uses reusable patterns
- A skill doesn't need to use primitives — many are standalone
- Document HOW the primitive is used, not just that it's referenced

### Relationships
- Map inputs/outputs to other skills
- This enables composite skill chains and `/discover-skills` recommendations
- Be specific about what data flows between skills

### Notes Section
- Include XOmnibus-specific architecture rules that affect this skill
- Note audio thread constraints if applicable
- Note parameter ID freeze rules if applicable
- Note preset compatibility concerns if applicable
