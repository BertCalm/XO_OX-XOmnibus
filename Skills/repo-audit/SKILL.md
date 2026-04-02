# Skill: /repo-audit

**Invoke with:** `/repo-audit`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** Monthly or after 3+ engine additions
**Purpose:** Comprehensive repo hygiene and documentation currency audit — finds stale counts, broken engine IDs, ODR hazards, duplicate files, missing governance docs, superseded doc notices, and stale skill metadata. Produces a prioritized fix list and executes all Phase 1–3 fixes in one pass.

---

## When to Use

- After adding 3+ new engines (counts go stale fast)
- Before a release (documentation must match code reality)
- When `master-audit` passes but docs feel off
- Monthly routine hygiene
- When the user says "scan the repo and clean things up"
- After a major Prism Sweep or quality campaign completes

**Distinction from `/master-audit`:**
`/master-audit` checks code correctness, doctrine compliance, and preset health.
`/repo-audit` checks *documentation accuracy*, *repo structure hygiene*, and *governance currency*.
Run both before a release. Run `/repo-audit` more frequently during active development.

---

## Phase 1 — Code Accuracy (Bugs That Look Like Docs Problems)

### 1.1 Engine ID Mismatch Detection

These are real runtime bugs, not just doc issues. Engine IDs must match exactly in three places:
1. `getEngineId()` return value in each adapter
2. Registration key in `Source/XOceanusProcessor.cpp`
3. `validEngineNames` set in `Source/Core/PresetManager.h`

```bash
# Extract all getEngineId() return values from adapters
grep -rn "getEngineId" Source/Engines/ | grep "return " | sort

# Extract all registerEngine() keys from processor
grep -n "registerEngine" Source/XOceanusProcessor.cpp | sort

# Extract validEngineNames list
grep -A 60 "validEngineNames" Source/Core/PresetManager.h | head -70
```

**Compare all three lists.** Any mismatch is a runtime bug — fix before proceeding.

**Known patterns to catch:**
- `"XOverlap"` vs `"Overlap"` — X-prefix leaking into ID
- `"Oceandeep"` vs `"OceanDeep"` — capitalization mismatch
- ID matches registration but not `validEngineNames` (causes preset load failure)
- `resolveEngineAlias()` masking a mismatch (alias handles preset loading but NOT runtime checks like `getSilencePeriod()`)

### 1.2 Duplicate File / ODR Hazard Detection

```bash
# Find duplicate engine headers (e.g. OceanDeepEngine.h AND OceandeepEngine.h)
find Source/Engines -name "*.h" | sed 's|.*/||' | sort -f | uniq -di

# Find duplicate .cpp files
find Source/Engines -name "*.cpp" | sed 's|.*/||' | sort -f | uniq -di

# Verify CMakeLists references match actual file names (case-sensitive)
grep -n "OceanDeep\|Oceandeep\|XOverlap\|XOutwit" CMakeLists.txt
```

If two files define the same class, delete the orphan (the one NOT included by the processor) and update `CMakeLists.txt`.

### 1.3 CMakeLists Consistency Check

```bash
# All engine headers in CMakeLists should exist on disk
grep "Source/Engines" CMakeLists.txt | grep "\.h" | while read line; do
  file=$(echo $line | tr -d ' \t\"')
  [ ! -f "$file" ] && echo "MISSING: $file"
done
```

---

## Phase 2 — Documentation Accuracy

### 2.1 Engine Count Staleness Scan

The canonical engine count is in CLAUDE.md header. Check it matches everywhere:

```bash
CANONICAL=$(grep -o '[0-9]* engines' CLAUDE.md | head -1 | grep -o '[0-9]*')
echo "Canonical count: $CANONICAL"

# Find stale counts in all docs
grep -rn "[0-9]* engines\|[0-9]* character instruments\|[0-9]* synthesis engines\|[0-9]* of [0-9]* engines" \
  Docs/ Skills/ README.md CLAUDE.md | grep -v "$CANONICAL"
```

Fix every mismatch found. Also check:
```bash
# Preset count (currently ~15,500)
grep -rn "10,000\|15,000\|15,200\|15,500\|10,028" Docs/ Skills/ README.md CLAUDE.md

# Mood count (should be 8 everywhere)
grep -rn "7 mood\|7 categories\|7 moods" Docs/ Skills/ README.md CLAUDE.md

# Coupling type count (should be 14)
grep -rn "12 coupling\|13 coupling\|Coupling Types (12)\|Coupling Types (13)" Docs/ CLAUDE.md
```

### 2.2 Engine Table Completeness

Verify `README.md` engine table, `Docs/xoceanus_master_specification.md` engine table, and CLAUDE.md engine list all contain the same engines:

```bash
# Count engine rows in README
grep -c "^| " README.md

# Count engine rows in master spec section 3.1
grep -A 200 "The 42 Engines" Docs/xoceanus_master_specification.md | grep -c "^|"

# List all engine directories that exist
ls Source/Engines/ | sort
```

For each engine in `Source/Engines/` that isn't in the tables, add it.

### 2.3 CLAUDE.md Completeness Checks

CLAUDE.md has 4 sections that must be updated together whenever an engine is added:

```bash
# 1. Engine modules list (comma-separated in header)
grep "Engine modules" CLAUDE.md

# 2. Engine Modules table (accent color rows)
grep -c "^| " CLAUDE.md  # rough count of table rows

# 3. Parameter Prefix table
grep "prefix_" CLAUDE.md | wc -l

# 4. Key Files table
grep "EngineH\|Engine.h" CLAUDE.md
```

Cross-check all 4 against the registered engine list. Missing rows = stale CLAUDE.md.

### 2.4 Blessings and Debates Currency

```bash
# Check blessing count in CLAUDE.md
grep "^| B[0-9]" CLAUDE.md | wc -l
# Should match the latest seance verdict files

# Check debate statuses
grep "^| DB" CLAUDE.md
# Any still UNRESOLVED that seance verdicts have resolved?

# Find seance verdict files for clues
ls Docs/seances/*.md 2>/dev/null | head -20
grep -l "RESOLVED\|BLESSED\|Blessing" Docs/seances/*.md 2>/dev/null
```

### 2.5 Seance Cross-Reference Currency

```bash
# Count seance rows
grep -c "^| [A-Z]" Docs/seances/seance_cross_reference.md

# List engine directories without a seance row
ls Source/Engines/ | while read engine; do
  grep -qi "$engine" Docs/seances/seance_cross_reference.md || echo "MISSING SEANCE: $engine"
done
```

### 2.6 Master Spec Section Audit

Key sections that commonly go stale:
- Section 1.1: Character instrument count
- Section 3.1: Engine table (add new engine rows)
- Section 4.1: Coupling type count and list
- Document Hierarchy: Living documents table

```bash
# Check coupling type count in master spec
grep "Coupling Types" Docs/xoceanus_master_specification.md

# Check for Design Doctrines section
grep "Design Doctrines\|2\.6" Docs/xoceanus_master_specification.md
```

---

## Phase 3 — Skills Audit

### 3.1 Coverage Gap Check

Map every workflow to a skill. Flag workflows that have no skill:

| Workflow | Expected Skill |
|----------|---------------|
| New engine ideation + scaffold | `/new-xo-engine` (external) |
| Adapter writing | No skill — consider creating |
| Seance evaluation | `/synth-seance` (external) |
| Post-engine completion | `/post-engine-completion-checklist` (external) |
| Preset creation | `/preset-architect` |
| Preset quality audit | `/preset-auditor` |
| DNA assignment | `/dna-designer` |
| Coupling preset design | `/coupling-preset-designer` |
| Coupling debugging | `/coupling-debugger` |
| Engine pairings reference | `/coupling-interaction-cookbook` |
| Mod matrix template | `/mod-matrix-builder` |
| Engine doctrine check | `/engine-health-check` |
| XPN/MPC export | `/xpn-export-specialist` |
| CPU optimization | `/sro-optimizer` |
| Fleet-wide QA | `/master-audit` |
| Repo hygiene audit | `/repo-audit` (this skill) |

```bash
ls Skills/*/SKILL.md | sort
```

Any workflow without a skill = candidate for a new skill file.

### 3.2 Skill Metadata Currency

Every `SKILL.md` should have these fields after `**Invoke with:**`:

```
**Status:** LIVE
**Last Updated:** YYYY-MM-DD | **Version:** X.Y | **Next Review:** <trigger>
**Purpose:** <one-line summary>
```

```bash
# Find skill files missing Last Updated
grep -rL "Last Updated" Skills/*/SKILL.md
```

### 3.3 Skills/README.md Currency

```bash
# Skills in directory but not in README table (checks link format, not body text)
for dir in Skills/*/; do
  skill=$(basename "$dir")
  grep -q "\[${skill}\](" Skills/README.md || echo "MISSING FROM README: $skill"
done

# README rows pointing to non-existent dirs
grep "\./.*SKILL\.md" Skills/README.md | grep -oP '\./\K[^/]+' | while read skill; do
  [ ! -d "Skills/$skill" ] && echo "DEAD LINK: $skill"
done
```

> **Note:** Use `grep -q "\[${skill}\]("` not `grep -q "$skill"` — the latter
> matches skill names appearing in body text (descriptions, related skills sections)
> and produces false "already in README" results for skills that are only mentioned
> in passing, not linked in the table.

---

## Phase 4 — Governance Audit

### 4.1 Required Governance Files

These files must exist:

```bash
[ -f Docs/GOVERNANCE.md ] && echo "OK: GOVERNANCE.md" || echo "MISSING: Docs/GOVERNANCE.md"
[ -f Docs/MANIFEST.md ] && echo "OK: MANIFEST.md" || echo "MISSING: Docs/MANIFEST.md"
[ -f Docs/documentation_health_plan.md ] && echo "OK: health plan" || echo "MISSING"
[ -f Docs/seances/seance_cross_reference.md ] && echo "OK: seance cross-ref" || echo "MISSING"
[ -f Docs/prism_sweep_final_report.md ] && echo "OK: prism sweep" || echo "MISSING"
```

### 4.2 Superseded Documents

Documents known to be superseded must have a deprecation header:

```bash
# Check for SUPERSEDED header in known stale docs
for f in Docs/xo_mega_tool_preset_system.md Docs/xo_mega_tool_visual_identity.md; do
  grep -q "SUPERSEDED" "$f" && echo "OK: $f" || echo "MISSING HEADER: $f"
done

# Scan Docs/ for large files that might be orphaned superseded docs
find Docs/ -name "*.md" -size +30k | sort -k5 -rh 2>/dev/null || \
find Docs/ -name "*.md" | xargs wc -l 2>/dev/null | sort -rn | head -20
```

### 4.3 Plans Status Headers

```bash
# Find plans without STATUS header
grep -rL "STATUS:" Docs/plans/
```

### 4.4 Concepts Directory Archive Notice

```bash
# Concepts dir should have archive notice
head -5 Docs/concepts/README.md 2>/dev/null || echo "Missing concepts/README.md"
grep -q "ARCHIVE\|archive" Docs/concepts/README.md 2>/dev/null || echo "MISSING ARCHIVE NOTICE: Docs/concepts/README.md"
```

---

## Phase 5 — Execute Fixes

Work through findings from Phases 1–4 in priority order:

### Priority Order

| Priority | Fix Type | Why |
|----------|----------|-----|
| P0 | Engine ID mismatches | Runtime bugs — silent failures |
| P0 | ODR hazards / duplicate files | Build failures or undefined behavior |
| P1 | CLAUDE.md count staleness | Source of truth must be accurate |
| P1 | Master spec stale engine table | Canonical reference doc |
| P2 | README.md stale counts/tables | User-facing, first impression |
| P2 | Skills README missing entries | Discoverability |
| P3 | Skill metadata (Last Updated etc.) | Housekeeping |
| P3 | Plans STATUS headers | Navigation |
| P3 | Superseded doc headers | Prevent confusion |

### Fix Templates

**Adding an engine row to a table (README.md or master spec):**
```markdown
| SHORTNAME | Source Instrument | Accent Color `#RRGGBB` |
```

**Deprecation header (superseded docs):**
```markdown
> ⚠️ **SUPERSEDED — DO NOT USE**
> This document was replaced by `Docs/{replacement_file}.md`.
> Kept for historical reference only. Content may be outdated or incorrect.
```

**Plans STATUS header:**
```markdown
> **STATUS: ACTIVE** | <brief description of current state>
```

**Skill metadata block (insert after `**Invoke with:**` line):**
```markdown
**Status:** LIVE
**Last Updated:** YYYY-MM-DD | **Version:** X.Y | **Next Review:** <trigger description>
```

---

## Phase 6 — Commit and Push

### Commit Message Template

```
docs: repo hygiene audit — {summary of main fixes}

Phase 1 — Code accuracy:
- {engine ID fix if any}
- {ODR/duplicate file fix if any}

Phase 2 — Documentation accuracy:
- {stale count fixes}
- {missing engine rows}
- {blessings/debates updates}

Phase 3 — Skills:
- {new skills or metadata added}

Phase 4 — Governance:
- {governance files created/updated}
- {deprecation headers added}
- {status headers added}
```

### Push

```bash
git add -p  # review each change
git commit -m "$(cat <<'EOF'
docs: repo hygiene audit — <summary>

<details>
EOF
)"
git push -u origin <branch>
```

---

## Quality Gate

Before marking `/repo-audit` complete:

- [ ] All `getEngineId()` returns match registration keys and `validEngineNames`
- [ ] No duplicate `.h`/`.cpp` files defining the same class
- [ ] CMakeLists.txt references only files that exist on disk
- [ ] Engine count consistent across CLAUDE.md, master spec, README.md, Skills
- [ ] CLAUDE.md has all 4 sections updated for every registered engine
- [ ] Blessings count in CLAUDE.md matches seance verdict files
- [ ] All resolved debates marked RESOLVED in CLAUDE.md
- [ ] `Docs/GOVERNANCE.md` exists and is current
- [ ] `Docs/MANIFEST.md` exists and covers all docs
- [ ] All superseded docs have deprecation headers
- [ ] All `Docs/plans/` files have STATUS headers
- [ ] All `Skills/*/SKILL.md` files have `Last Updated` metadata
- [ ] `Skills/README.md` lists every skill in `Skills/*/`
- [ ] Changes committed with descriptive message and pushed

---

## Related Skills

- `/master-audit` — QA/doctrine/preset health (complementary, run together pre-release)
- `/engine-health-check` — per-engine deep dive if Phase 1 finds issues
- `/coupling-debugger` — if Phase 1 finds ID mismatches causing coupling failures

## Related Docs

- `Docs/GOVERNANCE.md` — authority hierarchy, update checklists, SLAs
- `Docs/MANIFEST.md` — full documentation inventory
- `Docs/documentation_health_plan.md` — multi-phase improvement roadmap
- `CLAUDE.md` — source of truth for all counts and engine metadata
