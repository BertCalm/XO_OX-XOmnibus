# /nightly — Scheduled Maintenance Sweep

Comprehensive maintenance pass that goes beyond finding issues — it actively cleans, tidies, and improves the codebase. Think of it as the Roomba's deep-clean mode.

## Usage

- `/nightly` — Run full maintenance cycle
- `/nightly --dry-run` — Report what would be done without making changes
- `/nightly --report-only` — Just produce the health report, no fixes

## Maintenance Cycle

### Stage 1: Health Scan (read-only)

Run `/roomba quick` internally to get current health score and critical issues.

### Stage 2: Safe Auto-Fixes

Apply all fixes that are provably safe and non-breaking:

1. **CMakeLists.txt Sync**
   - Add any new .h/.cpp files found in Source/ that aren't in CMake
   - Remove any CMake entries pointing to files that no longer exist

2. **ValidEngineNames Sync**
   - Add any registered engines missing from the validEngineNames list
   - Add corresponding XFoo legacy aliases for new engines

3. **Preset Metadata Cleanup**
   - Fix obvious tag typos (Levenshtein distance ≤ 2 from known engine name)
   - Normalize whitespace in preset names
   - Fix mood casing inconsistencies

4. **Documentation Sync**
   - Update CLAUDE.md engine count if it doesn't match actual
   - Update Key Files table if paths have moved
   - Flag (don't auto-fix) any param prefix table mismatches

### Stage 3: Code Quality Pass

1. **Dead Import Scan**
   - Check each engine .cpp for includes that aren't needed
   - Check for #include cycles

2. **Comment Freshness**
   - Flag comments referencing old engine names (Snap, Morph, Drift, etc.)
   - Flag TODO/FIXME items older than 30 days (by git blame)

3. **Consistency Enforcement**
   - Verify all engine headers follow the same structural pattern
   - Check that every engine has matching .h and .cpp
   - Verify namespace usage is consistent

### Stage 4: Preset Garden

1. **Coverage Gaps**
   - Identify engines with < 10 presets
   - Flag moods with < 100 presets
   - Report suggested preset generation targets

2. **Quality Audit**
   - Flag presets where all macros map to zero change (dead macros)
   - Flag duplicate sonic DNA profiles (presets that sound identical)
   - Check that coupling presets actually reference multiple engines

### Stage 5: Report & Commit

Produce a maintenance report, then optionally commit all auto-fixes with message:
`nightly: maintenance sweep — {N} auto-fixes applied`

## Report Format

```markdown
# Nightly Maintenance Report
**Date:** {date} | **Health Score:** {before} → {after}

## Auto-Fixed
| # | Category | Fix | Files |
|---|----------|-----|-------|
{list of changes made}

## Needs Manual Attention
| # | Priority | Issue | Suggested Action |
|---|----------|-------|-----------------|
{list of items requiring human judgment}

## Codebase Health Trends
- Engines: {N} registered, {N} with presets, {N} with MPE
- Presets: {N} total across {N} moods
- Build: {status}
- Documentation: {drift score}

## Recommended Next Session Tasks
1. {highest priority manual fix}
2. {second priority}
3. {nice-to-have}
```

## Scheduling

This skill is designed to be run:
- At the start of each coding session (via session start reminder)
- Weekly as a deep maintenance pass
- After merging any PR
- Before release milestones

To set up as a recurring reminder, use: `/loop 24h /nightly --report-only`

## Philosophy

The nightly sweep follows the "campsite rule" — leave the codebase cleaner than you found it. Every session should improve overall health, even if the primary task is adding new features. Small, consistent maintenance prevents the "boiling frog" problem where drift accumulates until everything breaks at once.

## Notes

- Stage 2 fixes are all safe — they can't break parameter IDs, preset compatibility, or audio output
- Stage 3 and 4 are advisory — they report but don't auto-fix
- The health score trend is the key metric — it should never decrease session over session
- If health score drops below 70, the report will flag it prominently
