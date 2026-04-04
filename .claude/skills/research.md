# /research — Deep Feature & Technical Research

Investigate a topic thoroughly before building: read specs, scan codebase, check docs, understand current state, map constraints, and identify gaps.

## Usage

- `/research {topic}` — Research a specific feature, technique, or question
- `/research --scope codebase` — Focus on what exists in the code now
- `/research --scope spec` — Focus on what the spec says should exist
- `/research --scope external` — Focus on external references, techniques, prior art

## Process

### 1. Define the research question

Clarify what we're investigating. Ask the user if ambiguous:
- What specifically do you want to know?
- Are you evaluating feasibility, comparing approaches, or auditing completeness?
- What decision will this research inform?

### 2. Internal research (codebase)

**Spec layer:**
- Read `Docs/specs/xoceanus_master_specification.md` for relevant sections
- Read engine-specific docs if applicable (`xo_mega_tool_engine_catalog.md`, sound design guides)
- Read any related spec docs in `Docs/`
- Extract: what was *intended* vs what was *deferred* vs what was *rejected*

**Code layer:**
- Search `Source/` for existing implementations related to the topic
- Check parameter IDs, DSP modules, UI components
- Map dependencies: what touches this feature? What would change affect?
- Check `Tests/` for existing coverage of the area

**Tool layer:**
- Check `Tools/` for related utilities
- Check presets for real-world usage patterns of the feature

**History layer:**
- Check git log for relevant commits (`git log --oneline --grep="{topic}"`)
- Check for TODOs, FIXMEs, or commented-out code related to the topic

### 3. External research (if --scope external or relevant)

- Search for prior art, algorithms, techniques
- Reference academic papers or established DSP methods if applicable
- Check how other synths/plugins handle the same problem
- Note licensing constraints on any referenced implementations

### 4. Gap analysis

Compare what the spec says against what the code does:

| Aspect | Spec Says | Code Does | Gap |
|--------|-----------|-----------|-----|
| {feature} | {specified behavior} | {actual behavior or "not implemented"} | {description} |

### 5. Constraint mapping

Identify hard constraints that affect any future work:
- **Architecture:** Audio thread rules, parameter ID freezes, SynthEngine interface
- **Compatibility:** Existing presets that depend on current behavior
- **Performance:** CPU budget, memory limits, real-time requirements
- **Platform:** macOS/iOS differences, AU/AUv3 limitations
- **Design:** Gallery Model UI rules, brand conventions

### 6. Research Report

```markdown
## Research: {topic}
- **Question:** {what we investigated}
- **Scope:** codebase / spec / external / all

### Current State
{What exists today — code, docs, presets, tests}

### Spec Intent
{What the specification describes — may differ from current state}

### Gap Analysis
| Aspect | Spec | Code | Gap |
|--------|------|------|-----|
| ... | ... | ... | ... |

### Constraints
- {Hard constraints that limit options}

### Key Findings
1. {Most important discovery}
2. {Second most important}
3. {Third}

### Open Questions
- {Things we couldn't determine from available sources}

### Sources
| Source | Path | Relevant Section |
|--------|------|-----------------|
| {name} | {path} | {section or line range} |
```

## Primitives Used
- None directly — this is a pure research skill (read-only)

## Notes
- This skill is **read-only** — it never modifies files
- Research output is designed to feed into `/feature-review` and `/recommend`
- Keep findings concrete and source-referenced — no speculation without labeling it
- Flag contradictions between spec and code explicitly
- If the topic spans multiple engines, organize findings per engine
