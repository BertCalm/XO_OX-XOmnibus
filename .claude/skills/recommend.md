# /recommend — Prioritized Actionable Recommendations

Synthesize research and review findings into a ranked list of what to do next, with trade-offs, effort estimates, and impact assessments.

## Usage

- `/recommend` — Generate recommendations from recent research/review context
- `/recommend {area}` — Recommendations for a specific area (engine, feature, subsystem)
- `/recommend --roadmap` — Broader recommendations organized by timeline horizon
- `/recommend --quick-wins` — Only show low-effort, high-impact items

## Process

### 1. Gather inputs

Pull from available context:
- Recent `/research` findings (gaps, open questions, constraints)
- Recent `/feature-review` results (maturity scores, gaps, divergences)
- `/lint` results (architecture violations, quality issues)
- `/debug-audio` findings (DSP health issues)
- `/benchmark` results (performance regressions or headroom)
- `/preset-qa` results (preset quality gaps)
- Git history (recent direction of work, momentum)

If no recent context exists, run a lightweight `/research` and `/feature-review` first.

### 2. Score each potential action

For every gap, issue, or opportunity found, score on four dimensions:

| Dimension | Weight | Scale |
|-----------|--------|-------|
| **Impact** | 3× | How much does this improve the product for users? (1-5) |
| **Risk** | 2× | What's the cost of NOT doing this? (1-5, where 5 = high risk of not doing) |
| **Effort** | -1× | How much work? (1-5, where 5 = very large) |
| **Dependency** | 1× | Does this unblock other work? (1-5, where 5 = critical blocker) |

**Priority Score** = (Impact × 3) + (Risk × 2) - (Effort × 1) + (Dependency × 1)

### 3. Categorize

Group recommendations into:

**Must Do** (score > 20 or any critical safety/stability issue):
- Audio thread safety violations
- Broken preset compatibility
- Build failures
- Data loss risks

**Should Do** (score 12-20):
- Spec gaps that affect user experience
- Performance regressions
- Missing test coverage for shipped features
- Consistency violations

**Could Do** (score 6-12):
- Polish and refinement
- Nice-to-have features specified but not critical
- Code quality improvements
- Documentation gaps

**Defer** (score < 6):
- Speculative features
- Optimizations without measured need
- Cosmetic improvements

### 4. For each recommendation, provide

- **What:** Clear description of the action
- **Why:** The problem it solves or opportunity it captures
- **How:** High-level approach (not implementation detail)
- **Effort:** T-shirt size (S = hours, M = day, L = days, XL = week+)
- **Impact:** What changes for the user or developer after this is done
- **Trade-offs:** What you give up or risk by doing this
- **Prerequisites:** What must be done first
- **Validates with:** Which skill confirms this is done (`/test`, `/lint`, `/benchmark`, etc.)

### 5. Roadmap mode (if --roadmap)

Organize by time horizon:

**Now (this session/day):**
- Quick wins and blockers only
- Things that unblock other work

**Soon (this week):**
- High-priority spec gaps
- Quality improvements with clear ROI

**Next (this month):**
- Feature completeness
- Performance optimization
- Test coverage expansion

**Later (backlog):**
- Nice-to-haves
- Speculative exploration
- Long-term architectural improvements

### 6. Report

```markdown
## Recommendations: {area or "General"}
- **Based on:** {which inputs — research, review, lint, etc.}
- **Total items:** {count}

### Must Do
| # | Action | Why | Effort | Impact | Validates With |
|---|--------|-----|--------|--------|---------------|
| 1 | {what} | {why} | S/M/L/XL | {description} | /test, /lint |

### Should Do
| # | Action | Why | Effort | Impact | Validates With |
|---|--------|-----|--------|--------|---------------|
| 1 | {what} | {why} | S/M/L/XL | {description} | /benchmark |

### Could Do
| # | Action | Why | Effort | Impact | Validates With |
|---|--------|-----|--------|--------|---------------|
| 1 | {what} | {why} | S/M/L/XL | {description} | /review |

### Deferred
- {item} — Reason for deferral

### Dependencies
{Mermaid or text diagram showing which recommendations depend on others}

### Suggested Order of Execution
1. {first action} — unblocks {other actions}
2. {second action}
3. ...
```

### Quick-wins mode (if --quick-wins)

Filter to only items where:
- Effort = S (hours)
- Impact score >= 3
- No prerequisites / dependencies

Present as a flat numbered list:
```markdown
## Quick Wins
1. **{Action}** (S) — {one-line impact}
2. **{Action}** (S) — {one-line impact}
3. ...
```

## Primitives Used
- None directly — this is a synthesis/analysis skill (read-only)

## Relationship to Other Skills
- Consumes output from: `/research`, `/feature-review`, `/lint`, `/debug-audio`, `/benchmark`, `/preset-qa`
- Produces input for: actual implementation work, `/new-xo-engine`, `/preflight`
- Natural chain: `/research` → `/feature-review` → `/recommend` → build → `/preflight`

## Notes
- This skill is **read-only** — it recommends but doesn't implement
- Recommendations should be specific enough to act on without further research
- Always include trade-offs — every action has a cost
- Don't recommend what's already been decided against (check spec for explicit rejections)
- Quick-wins mode is designed for "I have 30 minutes, what's most valuable?"
- Roadmap mode is for planning sessions and sprint reviews
