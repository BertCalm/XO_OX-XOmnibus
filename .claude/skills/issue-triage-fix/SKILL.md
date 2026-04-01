---
name: issue-triage-fix
description: End-to-end codebase audit — scan for issues, create GitHub issues, assign model/skill recommendations, then fix by complexity tier (Haiku → Sonnet → Opus). Use when asked to audit the codebase, find and fix issues, or run a systematic quality pass.
argument-hint: [category e.g. dsp, security, docs, all]
---

# Issue Triage & Fix — Codebase Audit Skill

Perform a full audit of the XOlokun codebase, create GitHub issues for all findings, assign model/skill recommendations, and fix issues by complexity tier.

If `$ARGUMENTS` specifies a category (dsp, security, docs, tooling, website), focus on that category only. If "all" or no argument, scan everything.

## Full procedure

See `Skills/issue-triage-fix/SKILL.md` for the complete 6-phase workflow with scan commands, fix patterns, commit strategy, and quality gates.

---

## Phase 1 — Scan and Categorize

Scan the codebase across all major dimensions using parallel agents:

### DSP Safety
- Missing `flushDenormal()` after IIR smoothers (`smoothedX += (target - smoothedX) * coeff;`)
- Division-by-zero risks in audio paths (`/ variable` without `std::max` guard)
- Filter coefficient edge cases (`1.0f / denom` without guard)
- Feedback paths without denormal protection

### Security
- XML/HTML injection in export paths (Source/Export/)
- sys.argv mutation or shell injection in Python tools
- Bare except clauses, hardcoded developer paths

### Dead Code & Stale References
- Stale engine/coupling/preset counts in docs (canonical: 76 engines, 15 coupling types, ~17,250 presets)
- Dead parameters, unused JS, unreachable branches

### Website & Docs
- Missing Open Graph meta tags, robots.txt, sitemap.xml
- Broken internal links, stale counts in HTML

---

## Phase 2 — Create GitHub Issues

For each finding, create a GitHub issue:
```
Title: [Category] Brief description
Body: ## Problem, ## Location (file:line), ## Expected, ## Suggested Fix
```

---

## Phase 3 — Model/Skill Recommendation Comments

Add a comment to each issue:
```
**Model:** {Haiku|Sonnet|Opus}
**Skill:** {/skill-name or "None"}
**Rationale:** {One sentence}
```

### Model Assignment Guide
| Model | When |
|-------|------|
| **Haiku** | Mechanical pattern fix (add flushDenormal, fix count, add meta tag) |
| **Sonnet** | Context-aware, multi-file, design decisions needed |
| **Opus** | Architectural, new features, cross-cutting concerns |

---

## Phase 4 — Fix Issues by Complexity Tier

Work Haiku-tier first (fast, safe, high-volume), then Sonnet, then Opus.

### Common Haiku Fix Patterns

**Denormal flush:**
```cpp
smoothedX += (target - smoothedX) * coeff;
smoothedX = flushDenormal(smoothedX);  // ADD THIS
```

**Division-by-zero guard:**
```cpp
float result = (std::abs(range) < 1e-6f) ? 0.5f : value / range;
```

**Filter denominator guard:**
```cpp
float a1 = 1.0f / std::max(denom, 1e-6f);
```

**XML escape:**
```cpp
static juce::String xmlEscape(const juce::String& s) {
    return s.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace("\"","&quot;");
}
```

---

## Phase 5 — Commit Strategy

One commit per issue:
```bash
git add {specific files}
git commit -m "Brief fix description

Resolves #NNN"
```

Push after every 5-10 commits. Retry on network failure (4x exponential backoff).

---

## Phase 6 — PR and Verify

Create PR with structured body listing all fixes by category. Wait for CI to pass. Report summary table of created/fixed/remaining issues by category and model tier.
