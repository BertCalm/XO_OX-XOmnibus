# /feature-review — Feature Completeness & Quality Evaluation

Evaluate a feature against its spec, design intent, and quality standards. Answers: "Is this done? Is it good? What's missing?"

## Usage

- `/feature-review {feature}` — Review a specific feature or subsystem
- `/feature-review {engine}` — Review an engine's implementation completeness
- `/feature-review coupling` — Review coupling matrix implementation
- `/feature-review presets` — Review preset system completeness
- `/feature-review --all` — High-level review of all major subsystems

## Process

### 1. Establish the benchmark

Before reviewing, define what "complete" and "good" means for this feature.

**Sources of truth (in priority order):**
1. `Docs/specs/xoceanus_master_specification.md` — authoritative spec
2. Engine-specific docs in `Docs/` — detailed requirements
3. `CLAUDE.md` — architecture rules and conventions
4. Sound design guides — sonic intent and character expectations
5. Preset expectations — macros, DNA, coupling behavior

Extract a checklist of what the feature *should* do.

### 2. Implementation audit

For each requirement in the checklist:
- **Implemented:** Code exists and appears functional
- **Partial:** Code exists but incomplete or differs from spec
- **Missing:** No implementation found
- **Divergent:** Implementation exists but contradicts spec

Read the actual code — don't just grep for function names. Verify logic, not just presence.

### 3. Quality assessment

Beyond "does it exist," evaluate:

**Correctness:**
- Does the implementation match the spec's described behavior?
- Are edge cases handled (zero values, extreme ranges, null inputs)?
- Are there known bugs or TODOs in the code?

**Robustness:**
- Audio thread safety (no allocation, no blocking)
- Denormal protection where needed
- Graceful behavior with unexpected input

**Completeness:**
- All parameters defined and registered
- Preset compatibility maintained
- Coupling integration (if multi-engine feature)
- Test coverage exists

**Consistency:**
- Follows project conventions (naming, file structure, patterns)
- Consistent with how similar features work in other engines
- UI matches Gallery Model design system

**Sonic quality (for DSP features):**
- Dry sound is compelling before effects
- Macros produce audible, meaningful change
- Character matches the engine's described personality
- No artifacts (clicks, pops, zipper noise, DC offset)

### 4. Maturity scoring

Rate each dimension 1-5:

| Dimension | Score | Meaning |
|-----------|-------|---------|
| Correctness | 1-5 | 1=broken, 3=works with caveats, 5=spec-perfect |
| Robustness | 1-5 | 1=crashes, 3=handles common cases, 5=bulletproof |
| Completeness | 1-5 | 1=stub, 3=core done, 5=fully featured |
| Consistency | 1-5 | 1=convention violations, 3=mostly follows, 5=exemplary |
| Sonic Quality | 1-5 | 1=unusable, 3=functional, 5=inspiring |

**Overall maturity:**
- **Draft** (avg < 2): Needs fundamental work
- **Alpha** (avg 2-3): Core works but gaps and rough edges
- **Beta** (avg 3-4): Feature-complete with polish needed
- **Release** (avg 4-5): Ready for users

### 5. Report

```markdown
## Feature Review: {feature}
- **Maturity:** Draft / Alpha / Beta / Release
- **Overall Score:** {avg}/5

### Requirement Checklist
| # | Requirement | Status | Notes |
|---|------------|--------|-------|
| 1 | {from spec} | ✓ Implemented / ◐ Partial / ✗ Missing / ⚡ Divergent | {details} |
| 2 | ... | ... | ... |

### Maturity Scores
| Dimension | Score | Evidence |
|-----------|-------|----------|
| Correctness | {n}/5 | {why} |
| Robustness | {n}/5 | {why} |
| Completeness | {n}/5 | {why} |
| Consistency | {n}/5 | {why} |
| Sonic Quality | {n}/5 | {why} |

### Strengths
- {What's working well}

### Gaps
| Gap | Severity | Effort | Impact |
|-----|----------|--------|--------|
| {missing/partial item} | high/med/low | S/M/L | {user-facing effect} |

### Divergences from Spec
- {Where code differs from spec, and whether that's intentional or a bug}

### Dependencies
- {What other features/systems this touches}
- {What would break if this feature changed}
```

## Primitives Used
- None directly — this is an evaluation skill (read-only)

## Relationship to Other Skills
- Fed by `/research` — research findings inform the review
- Feeds into `/recommend` — review gaps become recommendation inputs
- Cross-references `/lint` — architecture rule compliance
- Cross-references `/debug-audio` — DSP quality aspects

## Notes
- This skill is **read-only** — it evaluates but doesn't modify
- Be honest about scores — inflated ratings undermine the tool's value
- "Divergent" is not automatically bad — sometimes code improves on spec
- When reviewing engines, load the sound design guide for character context
- For --all mode, produce a summary matrix, not full reviews of every feature
