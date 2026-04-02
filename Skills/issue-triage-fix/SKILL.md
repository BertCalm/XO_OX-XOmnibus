# Skill: /issue-triage-fix

**Invoke with:** `/issue-triage-fix`
**Status:** LIVE
**Last Updated:** 2026-03-31 | **Version:** 1.0 | **Next Review:** After next audit cycle or quarterly
**Purpose:** End-to-end codebase audit: scan for issues, create GitHub issues, assign model/skill recommendations, then fix issues in complexity order (Haiku → Sonnet → Opus) with one commit per fix.

---

## When to Use

- Periodic codebase health audit (monthly or pre-release)
- After a major development push when many small issues have accumulated
- When the user says "audit the codebase and fix everything"
- When you need to systematically triage and resolve a backlog of issues
- After adding multiple engines or features and want to catch loose ends

**Distinction from `/master-audit`:** `/master-audit` checks doctrine compliance and preset health. `/repo-audit` checks documentation accuracy and repo structure. `/issue-triage-fix` scans for *all* issue types (DSP bugs, security, dead code, stale docs, website problems, tooling), creates trackable GitHub issues, assigns complexity, and systematically resolves them.

---

## Phase 1 — Scan and Categorize

Scan the codebase across all major dimensions. Use parallel agents for speed.

### 1.1 DSP Safety Scan

```bash
# Missing denormal flushing in IIR smoothers
# Pattern: smoothedX += (target - smoothedX) * coeff; without flushDenormal after
grep -rn "smoothed.*+=.*target\|smoothed.*+=.*-.*smoothed" Source/Engines/ --include="*.h"

# Division-by-zero risks in audio paths
grep -rn "/ \(1\.\|denom\|range\|dx\|dy\)" Source/Engines/ --include="*.h" | grep -v "std::max\|epsilon\|1e-"

# Filter coefficient edge cases (SVF, ladder, biquad)
grep -rn "1.0f / " Source/Engines/ Source/DSP/ --include="*.h" | grep -v "std::max"

# Feedback paths without denormal protection
grep -rn "feedback\|delay\[.*\] =\|buffer\[.*\] =" Source/DSP/Effects/ --include="*.h"
```

### 1.2 Security Scan

```bash
# XML/HTML injection risks in export paths
grep -rn "<<.*name\|<<.*title\|<<.*preset" Source/Export/ --include="*.h"

# sys.argv mutation or shell injection in Python tools
grep -rn "sys.argv\|os.system\|subprocess.*shell=True" Tools/*.py

# Bare except clauses (mask real errors)
grep -rn "except:" Tools/*.py | grep -v "except: #"

# Hardcoded paths
grep -rn "/Users/\|/home/" Tools/*.py Source/ --include="*.py"
```

### 1.3 Dead Code and Stale References

```bash
# Dead parameters (declared but not wired to DSP)
# Use Phantom Sniff pattern: find params added via addParameter, check if loaded in renderBlock
grep -rn "addParameter\|createParameterLayout" Source/Engines/ --include="*.h" | head -50

# Stale engine counts in docs
grep -rn "[0-9]* engines\|[0-9]* synthesis engines" Docs/ site/ SDK/ README.md CLAUDE.md

# Stale coupling type counts
grep -rn "[0-9]* coupling" Docs/ site/ SDK/ README.md CLAUDE.md

# Dead JS in website
grep -rn "|| null;\|\/\/ TODO\|\/\/ FIXME\|\/\/ HACK" site/*.html site/**/*.html
```

### 1.4 Documentation and Website

```bash
# Missing Open Graph meta tags
for f in site/*.html site/**/*.html; do
  grep -qL "og:title" "$f" 2>/dev/null && echo "MISSING OG: $f"
done

# Missing robots.txt / sitemap.xml
[ -f site/robots.txt ] || echo "MISSING: robots.txt"
[ -f site/sitemap.xml ] || echo "MISSING: sitemap.xml"

# Broken internal links
grep -rn "href=\"[^h#]" site/*.html | grep -v "mailto:\|javascript:" | head -30
```

### 1.5 Tooling

```bash
# Python import errors or missing dependencies
python3 -c "import ast; [ast.parse(open(f).read()) for f in __import__('glob').glob('Tools/*.py')]" 2>&1

# Type errors in Python tools
grep -rn "def.*->.*:" Tools/*.py | head -20
```

### Category Taxonomy

Organize findings into these categories:

| Category | Examples |
|----------|---------|
| DSP Safety | Denormal flush, div-by-zero, filter guards |
| Security | XML injection, shell injection, bare excepts |
| Dead Code | Unused params, dead JS, unreachable branches |
| Stale Docs | Wrong engine counts, outdated coupling counts |
| Website | Missing meta tags, SEO, broken links |
| Tooling | Hardcoded paths, argv mutation, import errors |
| Architecture | Missing registrations, ID mismatches |

---

## Phase 2 — Create GitHub Issues

### 2.1 Issue Creation Pattern

For each finding, create a GitHub issue with:

```markdown
Title: [Category] Brief description of the problem
Body:
## Problem
{What's wrong and where}

## Location
{File path(s) and line number(s)}

## Expected
{What correct behavior looks like}

## Suggested Fix
{Concrete fix approach — enough for someone to implement without re-reading the code}
```

**Batch efficiently:** Create issues in parallel (up to 10 concurrent). Use `mcp__github__issue_write` tool.

**Labels:** Apply appropriate labels if the repo uses them (bug, documentation, enhancement, security).

### 2.2 Model/Skill Recommendation Comment

After creating each issue, add a comment with the recommended model and skill:

```markdown
**Model:** {Haiku|Sonnet|Opus}
**Skill:** {/skill-name or "None"}
**Rationale:** {One sentence explaining the complexity assignment}
```

#### Model Assignment Guide

| Complexity | Model | Characteristics |
|-----------|-------|-----------------|
| **Haiku** | Haiku | Mechanical, pattern-based fix. No architectural judgment needed. Examples: add `flushDenormal()` after IIR smoother, fix stale count, add meta tag, replace bare except, add epsilon guard |
| **Sonnet** | Sonnet | Requires understanding context, multiple files, or design decisions. Examples: refactor coupling route, add new preset mood, wire dead parameters requiring DSP knowledge, fix thread safety |
| **Opus** | Opus | Architectural decisions, new features, cross-cutting concerns. Examples: new engine design, coupling system redesign, major UI overhaul, performance optimization requiring profiling |

**Common misassignment traps:**
- Dead parameter wiring looks Haiku but often requires understanding the DSP graph → **Sonnet**
- "Update count" across many files looks Sonnet but is purely mechanical → **Haiku**
- Voice stealing / glide bugs require deep state machine knowledge → **Opus**
- Adding a missing feature (not just fixing a bug) → **Sonnet** minimum

---

## Phase 3 — Fix Issues by Complexity Tier

### 3.1 Haiku Tier (Mechanical Fixes)

Work through all Haiku-assigned issues first. These are fast, safe, and high-volume.

**Common Haiku fix patterns:**

#### Denormal Flushing
```cpp
// After any IIR smoother line like:
smoothedX += (target - smoothedX) * coeff;
// Add:
smoothedX = flushDenormal(smoothedX);

// If flushDenormal() isn't in scope, use inline:
smoothedX = (std::abs(smoothedX) < 1e-18f) ? 0.0f : smoothedX;
```

#### Division-by-Zero Guard
```cpp
// Before:
float result = value / range;
// After:
float result = (std::abs(range) < 1e-6f) ? 0.5f : value / range;
```

#### Filter Denominator Guard
```cpp
// Before:
float a1 = 1.0f / denom;
// After:
float a1 = 1.0f / std::max(denom, 1e-6f);
```

#### Stale Count Fix
Find-and-replace with verification:
```bash
# Verify canonical count first
grep "engines" CLAUDE.md | head -3
# Then fix all stale references
```

#### XML Escape
```cpp
static juce::String xmlEscape(const juce::String& s)
{
    return s.replace("&", "&amp;")
            .replace("<", "&lt;")
            .replace(">", "&gt;")
            .replace("\"", "&quot;");
}
```

#### Bare Except → Specific Exception
```python
# Before:
except:
# After:
except (ValueError, KeyError, FileNotFoundError) as e:
```

### 3.2 Sonnet Tier (Context-Aware Fixes)

After Haiku issues are complete, move to Sonnet-level issues. These require:
- Reading and understanding surrounding code
- Making design decisions within existing patterns
- Multi-file coordinated changes

### 3.3 Opus Tier (Architectural Fixes)

Save for dedicated sessions. These may require:
- User consultation on design direction
- Multiple related changes across the codebase
- New tests or verification strategies

---

## Phase 4 — Commit Strategy

### One Commit Per Issue

Each fix gets its own commit with a message that auto-closes the GitHub issue:

```bash
git add {specific files}
git commit -m "$(cat <<'EOF'
{Brief description of the fix}

Resolves #{issue_number}

{session URL}
EOF
)"
```

**Rules:**
- Stage specific files, never `git add -A`
- If a file is in `.gitignore`, use `git add -f {path}`
- Commit message starts with action verb: Fix, Add, Replace, Guard, Remove, Update
- Include `Resolves #NNN` to auto-close on merge
- One commit = one issue. Never batch multiple issue fixes into one commit.

### Push Strategy

Push after every 5-10 commits or at natural breakpoints:
```bash
git push -u origin {branch-name}
```

If push fails due to network, retry up to 4 times with exponential backoff (2s, 4s, 8s, 16s).

---

## Phase 5 — Verification and PR

### 5.1 Verify All Fixes

```bash
# Clean build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build

# AU validation
auval -v aumu Xolk XoOx

# Python tool syntax check
python3 -m py_compile Tools/oxport.py
python3 -m py_compile Tools/validate_presets.py
```

### 5.2 Create Pull Request

When all fixes for the session are committed and pushed:

```markdown
Title: {Brief scope}: {N} issues resolved across {categories}

Body:
## Summary
{2-3 bullet points covering the scope}

### {Category 1} ({N} fixes)
- **{Description}**: {details} (#{issue})
- ...

### {Category 2} ({N} fixes)
- ...

## Test plan
- [ ] Clean build passes
- [ ] auval passes
- [ ] {category-specific verification steps}
```

### 5.3 Model Reassignment Audit

After initial triage, review all model assignments. Common corrections:
- Issues assigned Haiku that require DSP context → upgrade to Sonnet
- Issues assigned Sonnet that are purely mechanical → downgrade to Haiku
- Issues assigned Haiku that involve voice stealing, thread safety, or architectural decisions → upgrade to Opus

Use `mcp__github__add_issue_comment` to update the recommendation comment on misassigned issues.

---

## Phase 6 — Reporting

After the session, summarize:

```markdown
## Audit Summary — {DATE}

### Issues Created: {N}
| Category | Created | Fixed | Remaining |
|----------|---------|-------|-----------|
| DSP Safety | X | Y | Z |
| Security | X | Y | Z |
| Dead Code | X | Y | Z |
| Stale Docs | X | Y | Z |
| Website | X | Y | Z |
| Tooling | X | Y | Z |

### Model Distribution
| Model | Assigned | Fixed This Session |
|-------|----------|-------------------|
| Haiku | X | Y |
| Sonnet | X | Y |
| Opus | X | Y |

### Next Steps
1. {Remaining Haiku fixes}
2. {Sonnet fixes to tackle next}
3. {Opus issues requiring user input}
```

---

## Quality Gate

Before marking `/issue-triage-fix` complete for a session:

- [ ] All scan categories covered (DSP, security, dead code, docs, website, tooling)
- [ ] Every finding has a GitHub issue with clear title and body
- [ ] Every issue has a model/skill recommendation comment
- [ ] Model assignments reviewed for accuracy (no Haiku on complex issues)
- [ ] All Haiku fixes committed with one-commit-per-issue pattern
- [ ] Each commit message includes `Resolves #NNN`
- [ ] Changes pushed to feature branch
- [ ] Build passes (if code was changed)
- [ ] PR created (if session is complete)
- [ ] Summary report produced

---

## Related Skills

- `/master-audit` — Doctrine compliance and preset health (complementary, different scope)
- `/repo-audit` — Documentation accuracy and repo structure (Phase 1.3/1.4 overlap)
- `/engine-health-check` — Per-engine deep dive for DSP issues found in Phase 1.1
- `/coupling-debugger` — For coupling-specific issues found during scan
- `/skill-friction-detective` — If this skill produces confusion or silent failures

## Related Docs

- `Docs/GOVERNANCE.md` — Authority hierarchy for issue prioritization
- `Docs/xolokun_master_specification.md` — Source of truth for correct counts/specs
- `CLAUDE.md` — Canonical engine list, parameter prefixes, architecture rules
